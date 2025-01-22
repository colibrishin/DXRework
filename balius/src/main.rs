use std::io::BufRead;
use std::io::Write;
use std::collections::HashSet;
use lazy_static::lazy_static;
use std::sync::Mutex;

lazy_static!{
    static ref target_files: Mutex<HashSet<String>> = Mutex::new(HashSet::new());
}


fn run_headerparser(engine_dir: &std::path::Path, intermediate_path: &std::path::Path)
{
    let parser_path = engine_dir.join("Programs").join("header-parser").join("MinSizeRel").join("header-parser.exe");
    
    for header in target_files.lock().unwrap().clone()
    {
        let mut parser = std::process::Command::new(&parser_path);
        println!("{}", intermediate_path.display());
        let output = parser.current_dir(intermediate_path).arg(header).arg("-e EENUM").arg("-c ECLASS").arg("-p EPROPERTY").arg("-f EFUNC").output().expect("Unable to spawn the process");
        println!("stdout: {}", String::from_utf8(output.stdout).unwrap());
        println!("stderr: {}", String::from_utf8(output.stderr).unwrap());
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

    if !intermediate_path.exists()
    {
        println!("Header parser seems not initialized...");
        std::fs::create_dir(&intermediate_path).expect("Unable to create a intermediate path");
        let gitignore_data : Vec<u8> = "target\nHeaderGenerated\\***".into();
        
        let gitignore_path = intermediate_path.join(".gitignore");
        let mut gitignore_file = std::fs::File::create(gitignore_path).expect("Unable to create a gitignore");
        gitignore_file.write(&gitignore_data).expect("Unable to write a gitignore file.");

        let command_to_run = vec![
            vec!["init"], 
            vec!["add", "."], 
            vec!["commit", "-m", "\"Init\""]];
        
        for command in command_to_run 
        {
            let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
            git_proc.current_dir(intermediate_path).args(command).status().expect("Repository initialization failed");
        }
    }
}

fn acquire_lock(win_dir: &std::path::Path) 
{
    loop
    {
        match std::fs::File::create("lock")
        {
            Ok(mut file) => 
            {
                let string_pid: Vec<u8> = std::process::id().to_string().into();
                file.write(&string_pid).expect("Unable to write a lock file");
                break;
            },
            Err(_) => match std::fs::File::open("lock") 
            {
                Ok(file) => 
                {
                    let mut reader = std::io::BufReader::new(file);
                    let mut pid = String::new();
                    reader.read_line(&mut pid).expect("Unable to read a lock file");
                    
                    let output = std::process::Command::new(win_dir.join("System32").join("tasklist.exe"))
                        .arg("/FI")
                        .arg(format!("PID eq {}", pid))
                        .output();
                    
                    match output 
                    {
                        Ok(_) => continue,
                        Err(_) => 
                        {
                            loop 
                            {
                                match std::fs::File::create("lock")
                                {
                                    Ok(mut retry_file) => 
                                    {
                                        let string_pid: Vec<u8> = std::process::id().to_string().into();
                                        retry_file.write(&string_pid).expect("Unable to write a lock file");
                                        break;
                                    },
                                    Err(_) => continue
                                }
                            }
                            
                            break;
                        }
                    }
                },
                Err(_) => continue
            }
        }
    }
}

fn main() 
{
    let args: Vec<String> = std::env::args().collect();

    if args.len() < 5
    {
        eprintln!("Insufficient arguments");
        return;
    }

    let engine_dir = std::path::Path::new(&args[1]);
    let project_name: &String = &args[2];
    let project_dir = std::path::Path::new(&args[3]);
    let git_dir = std::path::Path::new(&args[4]);
    let win_dir = std::path::Path::new(&args[5]);

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

    acquire_lock(&win_dir);

    let intermediate_path = engine_dir.join("Intermediate").join("HeaderParser");
    check_git(&git_dir, &intermediate_path);
    copy_headers(&intermediate_path, &project_name, &project_dir);
    diff_git(&git_dir, &intermediate_path);

    if !target_files.lock().unwrap().is_empty()
    {
        run_headerparser(&engine_dir, &intermediate_path);
    }
}
