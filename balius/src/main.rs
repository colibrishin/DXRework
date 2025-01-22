use std::io::Write;
use std::collections::HashSet;
use lazy_static::lazy_static;
use std::sync::Mutex;
use fs2::FileExt;
use std::io::BufRead;

lazy_static!{
    static ref target_files: Mutex<HashSet<String>> = Mutex::new(HashSet::new());
}

fn commit_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path) 
{
    let command_to_run = vec![
        vec!["config", "user.name", "header-parser"],
        vec!["config", "user.email", "fake@localhost"], 
        vec!["add", "."], 
        vec!["commit", "-m", "\"Auto commit\""]];
    
    for command in command_to_run 
    {
        let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
        git_proc.current_dir(intermediate_path).args(command).status().expect("commit failed");
    }
}

fn run_headerparser(engine_dir: &std::path::Path, intermediate_path: &std::path::Path)
{
    let parser_path = engine_dir.join("Programs").join("header-parser").join("Release").join("header-parser.exe");
    
    let mut parser = std::process::Command::new(&parser_path);
    let _output = parser.current_dir(intermediate_path).args(target_files.lock().unwrap().clone()).arg("-e EENUM").arg("-c ECLASS").arg("-p EPROPERTY").arg("-f EFUNC").output().expect("Unable to spawn the process");
    //println!("stdout: {}", String::from_utf8(output.stdout).unwrap());
    //println!("stderr: {}", String::from_utf8(output.stderr).unwrap());
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

fn copy_headers(intermediate_path: &std::path::Path, project_name: &String, project_dir: &std::path::Path)
{
    if project_dir.is_dir()
    {
        for entry in std::fs::read_dir(&project_dir).expect("Unable to read a project folder.")
        {
            let next_path = entry.expect("Not a valid path").path();
            
            if next_path.is_dir()
            {
                copy_headers(&intermediate_path, &project_name, &next_path);
            }

            if next_path.is_file() && next_path.extension().unwrap() == "h"
            {
                let dest = intermediate_path
                    .join(&project_name)
                    .join(next_path.with_extension("h").file_name().unwrap());

                println!("Found header: {}", dest.display());

                if !dest.parent().expect("Unable to get the parent path").exists()
                {
                    println!("Create a new folder for project...");
                    std::fs::create_dir(dest.parent().expect("Unable to get the parent path")).expect("Unable to create a parent path");
                }

                std::fs::copy(&next_path, &dest).expect("Unable to copy the header file");

                let generated_header = intermediate_path
                    .join("HeaderGenerated")
                    .join(&project_name)
                    .join(next_path.with_extension("generated.h").file_name().unwrap());

                if !generated_header.exists()
                {
                    println!("Header does not generated before, force regenerate...");
                    let path_without_intermediate = std::path::Path::new(project_name).join(next_path.with_extension("h").file_name().unwrap());
                    target_files.lock().unwrap().insert(path_without_intermediate.to_str().expect("Unable to translate to path").to_string());
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
                vec!["config", "user.name", "header-parser"],
                vec!["config", "user.email", "fake@localhost"],
                vec!["init"], 
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

fn acquire_lock()
{
    loop 
    {
        match std::fs::File::create_new("lock")
        {
            Ok(mut file) =>
            {
                match file.try_lock_exclusive()
                {
                    Ok(_) =>
                    {
                        let string_pid: Vec<u8> = std::process::id().to_string().into();
                        file.write(&string_pid).expect("Unable to write a lock file");
                        file.unlock().unwrap();
                        break;
                    },
                    Err(_) => continue
                }
            },
            Err(_) =>
            {
                match std::fs::File::open("lock")
                {
                    Ok(file) =>
                    {
                        match file.try_lock_exclusive()
                        {
                            Ok(_) =>
                            {
                                let mut reader = std::io::BufReader::new(&file);
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
                                                file.unlock().unwrap();
                                                match std::fs::remove_file("lock")
                                                {
                                                    Ok(()) => (),
                                                    Err(_) => (),
                                                }
                                                continue;
                                            },
                                            Some(_) => 
                                            {
                                                continue;
                                            }
                                        }
                                    },
                                    Err(_) =>
                                    {
                                        if pid.is_empty()
                                        {
                                            file.unlock().unwrap();
                                            match std::fs::remove_file("lock")
                                            {
                                                Ok(()) => (),
                                                Err(_) => (),
                                            }
                                            continue;
                                        }

                                        file.unlock().unwrap();
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
        }
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
    let project_name: &String = &args[2];
    let project_dir = std::path::Path::new(&args[3]);
    let git_dir = std::path::Path::new(&args[4]);

    if !engine_dir.exists() 
    {
        eprintln!("Engine directory does not exist");
        return;
    }

    if !project_dir.exists() 
    {
        eprintln!("Engine directory does not exist");
        return;
    }

    acquire_lock();

    let intermediate_path = engine_dir.join("Intermediate").join("HeaderParser");
    check_git(&git_dir, &intermediate_path);
    copy_headers(&intermediate_path, &project_name, &project_dir);
    diff_git(&git_dir, &intermediate_path);
    commit_git(&git_dir, &intermediate_path);

    if !target_files.lock().unwrap().is_empty()
    {
        run_headerparser(&engine_dir, &intermediate_path);
    }
}
