use std::io::Write;
use std::collections::HashSet;
use lazy_static::lazy_static;
use std::sync::Mutex;
use std::io::BufRead;

lazy_static!{
    static ref target_files: Mutex<HashSet<String>> = Mutex::new(HashSet::new());
}

fn write_target_file(intermediate_path: &std::path::Path) -> Result<(), Box<dyn std::error::Error>>
{
    let target_file_path = intermediate_path.join("target");
    let mut file = std::fs::File::options().create(true).write(true).truncate(true).open(&target_file_path)?;
    file.lock()?;

    let set = target_files.lock()?;
    for header_file in set.iter()
    {
        writeln!(file, "{}", &header_file)?;
    }

    file.unlock()?;
    Ok(())
}

fn commit_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path) -> Result<(), std::io::Error>
{
    let command_to_run = vec![
        vec!["add", "."], 
        vec!["commit", "-m", "\"Auto commit\""]];
    
    for command in command_to_run 
    {
        let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
        git_proc.current_dir(intermediate_path).args(command).output()?;
    }
    Ok(())
}

fn run_headerparser(engine_dir: &std::path::Path, intermediate_path: &std::path::Path, configuration: &String) -> Result<(), std::io::Error>
{
    let parser_path = engine_dir.join("Programs").join("header-parser").join("Release").join("header-parser.exe");
    let target_file = intermediate_path.join("target");

    let mut parser = std::process::Command::new(&parser_path);
    let _output = parser.current_dir(&intermediate_path).arg(&target_file).arg("-e EENUM").arg("-c ECLASS").arg("-p EPROPERTY").arg("-f EFUNC").arg("-m GENERATE_BODY").arg(format!("-b {}", configuration)).output()?;
    Ok(())
}

fn diff_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path) -> Result<(), Box<dyn std::error::Error>>
{
    let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
    let status_return = git_proc.current_dir(intermediate_path).args(["status", "--porcelain", "-uall"]).output()?;

    // no diff
    if status_return.stdout.is_empty()
    {
        println!("Git reports no diff");
        return Ok(());
    }

    let stdout_str = String::from_utf8(status_return.stdout)?;
    let lines : Vec<&str> = stdout_str.split("\n").collect();
    for line in lines 
    {
        if line.rfind(".h") == None 
        {
            continue;
        }
        let windows_style = line.replace("/","\\");
        let path = windows_style.split(" ").last().expect("panic: target file empty");
        println!("Git diff found: {}", path);
        let mut set = target_files.lock()?;
        set.insert(path.to_string());
    }

    Ok(())
}

fn copy_headers(intermediate_path: &std::path::Path, project_dir: &std::path::Path) -> Result<(), Box<dyn std::error::Error>>
{
    if project_dir.is_dir()
    {
        for entry in std::fs::read_dir(&project_dir)?
        {
            let next_path = entry?.path();
            
            if next_path.is_dir()
            {
                copy_headers(&intermediate_path, &next_path)?;
            }

            if next_path.is_file()
            {
                let extension = match next_path.extension()
                {
                    Some(ext) => ext,
                    None => continue
                };

                let target_intermediate = std::path::Path::new(next_path.parent().ok_or("parent does not exists")?);
                let mut project_root_path = std::path::Path::new("");
                
                'ancestor_find: for ancestor in target_intermediate.ancestors() 
                {
                    for entry in std::fs::read_dir(&ancestor)?
                    {
                        let build_cs_candidate = entry?.path();

                        if build_cs_candidate.is_file()
                        {
                            let filename = match build_cs_candidate.file_name()
                            {
                                Some(x) => x,
                                None => continue,
                            };
                            
                            if filename.to_str().ok_or("Unable to cast filename to string")?.ends_with("build.cs") 
                            {
                                project_root_path = ancestor;
                                break 'ancestor_find;
                            }
                        }
                    }
                }

                let project_name = project_root_path.iter().nth(project_root_path.iter().count() - 1).ok_or("project root path is far ahead of root directory")?;
                let sub_directory_and_filename = next_path.strip_prefix(project_root_path.parent().ok_or("parent does not exists")?)?;
                let sub_directory_and_filename_str = sub_directory_and_filename.to_str().ok_or("unable to cast subdirectory filename to string")?;
                
                if extension == "h"
                {
                    let dest = intermediate_path.join(sub_directory_and_filename.with_extension("h"));
                    let dest_parent = dest.parent().ok_or("destination parent does not exists")?;

                    println!("Candidate header: {}", sub_directory_and_filename.display());

                    if !dest_parent.exists()
                    {
                        println!("Create a new folder for project...");
                        std::fs::create_dir_all(dest_parent)?;
                    }

                    std::fs::copy(&next_path, &dest)?;

                    let generated_filename_with_extension = sub_directory_and_filename.with_extension("generated.h");
                    let generated_filename = generated_filename_with_extension.file_name().ok_or("unable to cast filename to string")?;
                    let generated_header = intermediate_path
                        .join("HeaderGenerated")
                        .join(&project_name)
                        .join(&generated_filename);

                    if !generated_header.exists()
                    {
                        println!("{}", generated_header.display());
                        println!("Header does not generated before, force regenerate...");
                        let mut set = target_files.lock()?;
                        set.insert(sub_directory_and_filename_str.to_string());
                    }
                }
                
                if extension == "dep"
                {
                    let next_path_filename = match next_path.file_name()
                    {
                        Some(filename) => filename,
                        None => continue,
                    };

                    let dependency_dst = intermediate_path
                        .join(&project_name)
                        .join(next_path_filename);
                    let dependency_dst_parent = dependency_dst.parent().ok_or("unable to get the parent path for dependency")?;

                    if !dependency_dst_parent.exists()
                    {
                        println!("Create a new folder for project...");
                        std::fs::create_dir_all(dependency_dst_parent)?;
                    }

                    if !dependency_dst.exists()
                    {
                        std::fs::copy(&next_path, &dependency_dst)?;
                    }
                }
            }
        }
    }

    Ok(())
}

fn check_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path) -> Result<(), std::io::Error>
{
    println!("Intermediate Path: {}", intermediate_path.display());

    if !std::fs::exists(&intermediate_path)?
    {
        println!("Header parser seems to be not initialized...");
        std::fs::create_dir(&intermediate_path)?;
        
        let gitignore_data : &[u8] = "target\nHeaderGenerated\\***\n*.generated.h\n".as_bytes();
        let gitignore_path = intermediate_path.join(".gitignore");

        let mut file = std::fs::File::create_new(gitignore_path)?;
        file.lock()?;
        file.write(&gitignore_data)?;
        file.unlock()?;

        let command_to_run = vec![
        vec!["init"], 
        vec!["config", "user.name", "header-parser"],
        vec!["config", "user.email", "fake@localhost"],
        vec!["add", "."], 
        vec!["commit", "-m", "\"Init\""]];
        
        for command in command_to_run 
        {
            let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
            git_proc.current_dir(intermediate_path).args(command).output()?;
        }
    }

    Ok(())
}

fn acquire_lock() -> Result<std::fs::File, std::io::Error>
{
    let mut lockfile = std::fs::File::options().read(true).append(true).create(true).open("lock")?;
    
    'retry: loop 
    {
        match lockfile.try_lock()
        {
            Ok(true) => 
            {
                let mut reader = std::io::BufReader::new(&lockfile);
                let mut pid = String::new();
                reader.read_line(&mut pid)?;
            
                let sys = sysinfo::System::new_all();
                match pid.parse::<usize>()
                {
                    Ok(parse_pid) =>
                    {
                        match sys.process(sysinfo::Pid::from(parse_pid))
                        {
                            None =>
                            {
                                let string_pid = std::process::id().to_string();
                                lockfile.write_all(string_pid.as_bytes())?;
                                break 'retry;
                            },
                            Some(_) => 
                            {
                                lockfile.unlock()?;
                                continue;
                            }
                        }
                    },
                    Err(_) =>
                    {
                        if pid.is_empty()
                        {
                            let string_pid = std::process::id().to_string();
                            lockfile.write_all(string_pid.as_bytes())?;
                            break 'retry;
                        }

                        lockfile.unlock()?;
                        continue;
                    }
                }
            },
            Ok(false) => continue,
            Err(_) => panic!("unable to acquire a lock file")
        }
    }

    println!("Lock acquired");
    return Ok(lockfile);
}

fn prepare_and_commit(engine_dir: &std::path::Path, project_dir: &std::path::Path, git_dir: &std::path::Path, intermediate_path: &std::path::Path, configuration: &String) -> Result<(), Box<dyn std::error::Error>>
{
    println!("Check git repository");
    check_git(&git_dir, &intermediate_path)?;
    println!("Copy new headers");
    copy_headers(&intermediate_path, &project_dir)?;
    println!("Check diff with git");
    diff_git(&git_dir, &intermediate_path)?;
    println!("Commit diff to git");
    commit_git(&git_dir, &intermediate_path)?;

    if !target_files.lock()?.is_empty()
    {
        println!("Write target file");
        write_target_file(&intermediate_path)?;
        println!("Starts header parser");
        run_headerparser(&engine_dir, &intermediate_path, &configuration)?;
    }
    Ok(())
}

fn main() 
{
    let args: Vec<String> = std::env::args().collect();

    if args.len() < 4
    {
        eprintln!("Insufficient arguments");
        return;
    }

    let engine_dir = std::path::Path::new(&args[1]);
    let project_dir = std::path::Path::new(&args[3]);
    let git_dir = std::path::Path::new(&args[4]);
    let configuration = &args[5];

    if !engine_dir.exists() 
    {
        eprintln!("Engine directory does not exist");
        return;
    }

    if !project_dir.exists() 
    {
        eprintln!("Project directory does not exist");
        return;
    }

    let intermediate_path = engine_dir.join("Intermediate").join("HeaderParser");
    
    println!("Acquiring lock");
    let lockfile = match acquire_lock()
    {
        Ok(file) => file,
        Err(_) => panic!("Unable to lock the file"),
    };

    match prepare_and_commit(&engine_dir, &project_dir, &git_dir, &intermediate_path, &configuration)
    {
        Ok(_) => lockfile.unlock().unwrap(),
        Err(_) =>
        {
            eprintln!("Unable to process the headers");
            lockfile.unlock().unwrap();
        }
    }
}
