use std::io::BufRead;
use std::io::Write;

static mut target_files : Vec<String> = vec![];

fn diff_git(git_proc: &mut std::process::Command) 
{
    let status_return = git_proc.args(["status", "--porcleain", "-uall"]).output().expect("git status failed");

    // no diff
    if status_return.stdout.is_empty()
    {
        println!("Git reports no diff");
        return;
    }
}

fn copy_headers(intermediate_path: &std::path::Path, project_name: &String, project_dir: &std::path::Path)
{
    println!("Searching headers: {}", project_dir.display());
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
                    .join(project_name)
                    .join(next_path.with_extension("h").file_name().unwrap());

                println!("Found header: {}", dest.display());
                std::fs::copy(&next_path, &dest).expect("Unable to copy the header file");

                let generated_header = intermediate_path
                    .join("HeaderGenerated")
                    .join(&project_name)
                    .join(next_path.with_extension("generated.h").file_name().unwrap());

                unsafe
                {
                    if !generated_header.exists()
                    {
                        println!("Header does not generated before, force regenerate...");
                        target_files.push(generated_header.to_str().expect("Invalid header path").to_string());
                    }
                } 
            }
        }
    }
}

fn check_git(intermediate_path: &std::path::Path, git_proc: &mut std::process::Command) 
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
            vec!["commit", "-m", "Init"]];
        
        for command in command_to_run.iter() 
        {
            git_proc.args(command).status().expect("Repository initialization failed");
        }
    }
}

fn acquire_lock() 
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
                    
                    let output = std::process::Command::new("tasklist")
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
    let mut git_proc = std::process::Command::new("git");

    let args: Vec<String> = std::env::args().collect();

    if args.len() < 3
    {
        eprintln!("Insufficient arguments");
        return;
    }

    let engine_dir = std::path::Path::new(&args[1]);
    let project_name: &String = &args[2];
    let project_dir = std::path::Path::new(&args[3]);

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
    check_git(&intermediate_path, &mut git_proc);
    copy_headers(&intermediate_path, &project_name, &project_dir);
    diff_git(&mut git_proc);

}
