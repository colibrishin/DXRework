use std::io::Write;
use std::collections::HashSet;
use lazy_static::lazy_static;
use std::sync::Mutex;
use fs2::FileExt;
use std::io::BufRead;

lazy_static!{
    static ref target_files: Mutex<HashSet<String>> = Mutex::new(HashSet::new());
}

fn write_target_file(intermediate_path: &std::path::Path)
{
    let target_file_path = intermediate_path.join("target");
    let mut file = std::fs::File::create(&target_file_path).expect("Unable to create a target file");
    for header_file in target_files.lock().unwrap().iter()
    {
        writeln!(file, "{}", &header_file).expect("Unable to write a target file");
    }
}

fn commit_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path) 
{
    let command_to_run = vec![
        vec!["add", "."], 
        vec!["commit", "-m", "\"Auto commit\""]];
    
    for command in command_to_run 
    {
        let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
        git_proc.current_dir(intermediate_path).args(command).status().expect("commit failed");
    }
}

fn run_headerparser(engine_dir: &std::path::Path, intermediate_path: &std::path::Path, configuration: &String)
{
    let parser_path = engine_dir.join("Programs").join("header-parser").join("Release").join("header-parser.exe");
    let target_file = intermediate_path.join("target");

    let mut parser = std::process::Command::new(&parser_path);
    let _output = parser.current_dir(&intermediate_path).arg(&target_file).arg("-e EENUM").arg("-c ECLASS").arg("-p EPROPERTY").arg("-f EFUNC").arg("-m GENERATE_BODY").arg(format!("-b {}", configuration)).output().expect("Unable to spawn the process");
    
    match std::str::from_utf8(&_output.stdout)
    {
        Ok(out) => 
        {
            println!("{}", out);
        },
        Err(_) => {}
    }

    match std::str::from_utf8(&_output.stderr) 
    {
        Ok(out) =>
        {
            println!("{}", out);
        },
        Err(_) => {}
    }
}

fn diff_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path) 
{
    let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
    let status_return = git_proc.current_dir(intermediate_path).args(["status", "--porcelain", "-uall"]).output().expect("git status failed");

    // no diff
    if status_return.stdout.is_empty()
    {
        println!("Git reports no diff");
        return;
    }

    let stdout_str = String::from_utf8(status_return.stdout).expect("Unknown character input found");
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
        target_files.lock().unwrap().insert(path.to_string());
    }
}

fn copy_headers(intermediate_path: &std::path::Path, project_dir: &std::path::Path)
{
    if project_dir.is_dir()
    {
        for entry in std::fs::read_dir(&project_dir).expect("Unable to read a project folder.")
        {
            let next_path = entry.expect("Not a valid path").path();
            
            if next_path.is_dir()
            {
                copy_headers(&intermediate_path, &next_path);
            }

            if next_path.is_file()
            {
                let extension = match next_path.extension()
                {
                    Some(ext) => ext,
                    None => continue
                };

                let target_intermediate = std::path::Path::new(next_path.parent().expect("Parent path does not exists"));
                let mut project_root_path = std::path::Path::new("");
                
                'ancestor_find: for ancestor in target_intermediate.ancestors() 
                {
                    for entry in std::fs::read_dir(&ancestor).expect("Unable to find the parent folder")
                    {
                        let build_cs_candidate = entry.expect("Not a valid path").path();

                        if build_cs_candidate.is_file() && build_cs_candidate.file_name().expect("File name not found").to_str().expect("Unable to cast to string").ends_with("build.cs")
                        {
                            project_root_path = ancestor;
                            break 'ancestor_find;
                        }
                    }
                }

                let project_name = project_root_path.iter().nth(project_root_path.iter().count() - 1).expect("Unable to parse the project name");
                let sub_directory_and_filename = next_path.strip_prefix(project_root_path.parent().expect("Drive root reached")).expect("Project path is not compatible with header file path");
                
                if extension == "h"
                {
                    let dest = intermediate_path.join(sub_directory_and_filename.with_extension("h"));

                    println!("Candidate header: {}", sub_directory_and_filename.display());

                    if !dest.parent().expect("Unable to get the parent path").exists()
                    {
                        println!("Create a new folder for project...");
                        std::fs::create_dir_all(dest.parent().expect("Unable to get the parent path")).expect("Unable to create a parent path");
                    }

                    std::fs::copy(&next_path, &dest).expect("Unable to copy the header file");

                    let generated_header = intermediate_path
                        .join("HeaderGenerated")
                        .join(&project_name)
                        .join(sub_directory_and_filename.with_extension("generated.h").file_name().unwrap());

                    if !generated_header.exists()
                    {
                        println!("{}", generated_header.display());
                        println!("Header does not generated before, force regenerate...");
                        target_files.lock().unwrap().insert(sub_directory_and_filename.to_str().expect("Unable to translate to path").to_string());
                    }
                }
                
                if extension == "dep"
                {
                    let dependency_dst = intermediate_path
                        .join(&project_name)
                        .join(next_path.file_name().unwrap());

                    if !dependency_dst.parent().expect("Unable to get the parent path").exists()
                    {
                        println!("Create a new folder for project...");
                        std::fs::create_dir_all(dependency_dst.parent().expect("Unable to get the parent path")).expect("Unable to create a parent path");
                    }

                    if !dependency_dst.exists()
                    {
                        std::fs::copy(&next_path, &dependency_dst).expect("Unable to copy the dependency file");
                    }
                }
            }
        }
    }
}

fn check_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path) 
{
    println!("Intermediate Path: {}", intermediate_path.display());
    
    let init_trap = || 
        {
            let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
            git_proc.current_dir(intermediate_path).args(["rev-parse", "HEAD"]);
            loop 
            {
                match git_proc.status()
                {
                    Ok(status) => 
                    {
                        if status.success()
                        {
                            break;
                        }
                    },
                    Err(_) => continue
                }
            }
        };

    if !intermediate_path.exists()
    {
        println!("Header parser seems to be not initialized...");

        match std::fs::create_dir(&intermediate_path)
        {
            Ok(()) => (),
            Err(_) => 
            {
                init_trap();
            }
        };

        let gitignore_data : &[u8] = "target\nHeaderGenerated\\***\n*.generated.h\n".as_bytes();
        let gitignore_path = intermediate_path.join(".gitignore");

        match std::fs::File::create_new(gitignore_path)
        {
            Ok(mut file) =>
            {
                file.lock_exclusive().expect("Unable to lock the file");
                file.write(&gitignore_data).expect("Unable to write a gitignore file.");
                file.unlock().expect("Unable to unlock the file");

                let command_to_run = vec![
                vec!["init"], 
                vec!["config", "user.name", "header-parser"],
                vec!["config", "user.email", "fake@localhost"],
                vec!["add", "."], 
                vec!["commit", "-m", "\"Init\""]];
                
                for command in command_to_run 
                {
                    let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
                    git_proc.current_dir(intermediate_path).args(command).status().expect("Repository initialization failed");
                }
            }
            Err(_) =>
            {
                init_trap();
            }
        }; 
    }
    else 
    {
        init_trap();
    }
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

    let mut lockfile;
    loop 
    {
        match std::fs::File::options().read(true).append(true).create(true).open("lock")
        {
            Ok(file) =>
            {
                match file.try_lock_exclusive()
                {
                    Ok(_) => 
                    {
                        lockfile = file;

                        let mut reader = std::io::BufReader::new(&lockfile);
                        let mut pid = String::new();
                        reader.read_line(&mut pid).expect("Unable to read a file");
                    
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
                                        std::fs::remove_file("lock").unwrap();
                                        lockfile.unlock().unwrap();
                                        continue;
                                    },
                                    Some(_) => continue
                                }
                            },
                            Err(_) =>
                            {
                                if pid.is_empty()
                                {
                                    let string_pid = std::process::id().to_string();
                                    lockfile.write_all(string_pid.as_bytes()).expect("Unable to write a lock file");
                                    break;
                                }
                                continue;
                            }
                        }
                    },
                    Err(_) => continue
                }

            },
            Err(_) => continue
        }
    }

    check_git(&git_dir, &intermediate_path);
    copy_headers(&intermediate_path, &project_dir);
    diff_git(&git_dir, &intermediate_path);
    commit_git(&git_dir, &intermediate_path);

    if !target_files.lock().unwrap().is_empty()
    {
        write_target_file(&intermediate_path);
        run_headerparser(&engine_dir, &intermediate_path, &configuration);
    }

    lockfile.unlock().unwrap();
}
