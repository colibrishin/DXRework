use std::io::{Seek, SeekFrom, Write};
use std::collections::HashSet;
use lazy_static::lazy_static;
use std::sync::Mutex;
use std::io::BufRead;
use std::fs::TryLockError;
use sha2::{Sha256, Digest};

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

/// Commit in the repo at intermediate_path. If paths_to_add is empty, runs "git add ." (per-project repo);
/// otherwise adds only the given paths (relative to intermediate_path).
fn commit_git(git_dir: &std::path::Path, intermediate_path: &std::path::Path, paths_to_add: &[String]) -> Result<(), std::io::Error>
{
    if paths_to_add.is_empty()
    {
        let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
        git_proc.current_dir(intermediate_path).args(["add", "."]).output()?;
    }
    else
    {
        for path in paths_to_add
        {
            let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
            git_proc.current_dir(intermediate_path).args(["add", path]).output()?;
        }
    }
    let mut git_proc = std::process::Command::new(git_dir.join("cmd").join("git.exe"));
    git_proc.current_dir(intermediate_path).args(["commit", "-m", "\"Auto commit\""]).output()?;
    Ok(())
}

fn run_headerparser(engine_dir: &std::path::Path, intermediate_path: &std::path::Path, configuration: &String) -> Result<(), std::io::Error>
{
    let parser_path = engine_dir.join("Programs").join("header-parser").join("Release").join("header-parser.exe");
    if !parser_path.exists() {
        eprintln!("header-parser exe not found: {}", parser_path.display());
        eprintln!("Generated headers will not be created. Build header-parser (e.g. Release config) or run CMake in Programs/header-parser.");
        return Err(std::io::Error::new(std::io::ErrorKind::NotFound, format!("header-parser not found: {}", parser_path.display())));
    }
    let target_file = intermediate_path.join("target");
    println!("HeaderGenerated output: {}", intermediate_path.join("HeaderGenerated").display());

    let mut child = std::process::Command::new(&parser_path)
        .current_dir(intermediate_path)
        .arg(&target_file)
        .arg("-e EENUM")
        .arg("-c ECLASS")
        .arg("-p EPROPERTY")
        .arg("-f EFUNC")
        .arg("-m GENERATE_BODY")
        .arg(format!("-b {}", configuration))
        .stdout(std::process::Stdio::piped())
        .stderr(std::process::Stdio::piped())
        .spawn()?;

    let mut stdout = child.stdout.take().ok_or_else(|| std::io::Error::new(std::io::ErrorKind::Other, "no stdout"))?;
    let mut stderr = child.stderr.take().ok_or_else(|| std::io::Error::new(std::io::ErrorKind::Other, "no stderr"))?;

    let stdout_handle = std::thread::spawn(move || std::io::copy(&mut stdout, &mut std::io::stdout().lock()));
    let stderr_handle = std::thread::spawn(move || std::io::copy(&mut stderr, &mut std::io::stderr().lock()));

    let _ = stdout_handle.join();
    let _ = stderr_handle.join();
    let status = child.wait()?;
    if !status.success() {
        eprintln!("header-parser exited with non-zero status");
        return Err(std::io::Error::new(std::io::ErrorKind::Other, "header-parser exited with non-zero status"));
    }
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
        let line_normalized = line.replace("/", "\\");
        let Some(path) = line_normalized.split_whitespace().last() else { continue };
        let is_header = path.ends_with(".h") || path.ends_with(".hpp");
        let is_cpp = path.ends_with(".cpp");
        if !is_header && !is_cpp
        {
            continue;
        }
        println!("Git diff found: {}", path);
        let mut set = target_files.lock()?;
        set.insert(path.replace('\\', "/"));
    }

    Ok(())
}

/// Copy headers (and .dep) from project_dir to intermediate_path. Appends paths we write to paths_copied
/// (relative to intermediate_path, forward slashes) for later git add only those.
fn copy_headers(intermediate_path: &std::path::Path, project_dir: &std::path::Path, paths_copied: &mut Vec<String>) -> Result<(), Box<dyn std::error::Error>>
{
    if project_dir.is_dir()
    {
        for entry in std::fs::read_dir(&project_dir)?
        {
            let next_path = entry?.path();
            
            if next_path.is_dir()
            {
                copy_headers(&intermediate_path, &next_path, paths_copied)?;
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

                let _project_name = project_root_path.iter().nth(project_root_path.iter().count() - 1).ok_or("project root path is far ahead of root directory")?;
                // Path relative to project root (intermediate_path is per-project, so no project_name in path).
                let sub_directory_and_filename = next_path.strip_prefix(&project_root_path).map_err(|_| "path not under project root")?;
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
                    paths_copied.push(sub_directory_and_filename.with_extension("h").to_string_lossy().replace('\\', "/"));

                    // header-parser writes to HeaderGenerated/Public|Private|<first_dir>/<name>.generated.h (see GetDestinationPath in main.cc).
                    // When path has no directory (e.g. Client.h), parser uses Public; otherwise first component.
                    let generated_filename_with_extension = sub_directory_and_filename.with_extension("generated.h");
                    let generated_filename = generated_filename_with_extension.file_name().ok_or("unable to cast filename to string")?;
                    let first_dir = sub_directory_and_filename.components().next().and_then(|c| match c {
                        std::path::Component::Normal(p) => Some(p.to_owned()),
                        _ => None,
                    });
                    let generated_header = match &first_dir {
                        Some(d) if sub_directory_and_filename != std::path::Path::new(d) =>
                            intermediate_path.join("HeaderGenerated").join(d).join(&generated_filename),
                        _ => intermediate_path.join("HeaderGenerated").join("Public").join(&generated_filename),
                    };

                    if !generated_header.exists()
                    {
                        println!("{}", generated_header.display());
                        println!("Header does not generated before, force regenerate...");
                        let mut set = target_files.lock()?;
                        set.insert(sub_directory_and_filename_str.replace('\\', "/"));
                    }
                }
                
                if extension == "dep"
                {
                    let next_path_filename = match next_path.file_name()
                    {
                        Some(filename) => filename,
                        None => continue,
                    };

                    let dependency_dst = intermediate_path.join("dep").join(next_path_filename);
                    let dependency_dst_parent = dependency_dst.parent().ok_or("unable to get the parent path for dependency")?;

                    if !dependency_dst_parent.exists()
                    {
                        println!("Create a new folder for project...");
                        std::fs::create_dir_all(dependency_dst_parent)?;
                    }

                    if !dependency_dst.exists()
                    {
                        std::fs::copy(&next_path, &dependency_dst)?;
                        paths_copied.push(format!("dep/{}", next_path_filename.to_string_lossy()));
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

    let git_dir_marker = intermediate_path.join(".git");
    if !std::fs::exists(&intermediate_path)?
    {
        std::fs::create_dir_all(&intermediate_path)?;
    }
    if !std::fs::exists(&git_dir_marker)?
    {
        println!("Header parser repo not initialized, initializing...");
        // Only ignore the target file list; commit copied headers and HeaderGenerated (per-project repo).
        let gitignore_data : &[u8] = "target\n".as_bytes();
        let gitignore_path = intermediate_path.join(".gitignore");

        let mut file = std::fs::File::options().write(true).create_new(true).open(gitignore_path)?;
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

/// Lock file: one per engine tree, under Intermediate/HeaderParser. PID is stored inside the file (first line)
/// for stale-lock detection; the filename is always "lock" (no PID in filename).
/// Overwrites the lock file with the current process PID (first line). Call only while holding the lock.
fn write_lock_pid(lockfile: &mut std::fs::File) -> Result<(), std::io::Error> {
    let string_pid = std::process::id().to_string();
    lockfile.seek(SeekFrom::Start(0))?;
    lockfile.write_all(string_pid.as_bytes())?;
    lockfile.write_all(b"\n")?;
    let pos = lockfile.seek(SeekFrom::Current(0))?;
    lockfile.set_len(pos)?;
    lockfile.flush()?;
    Ok(())
}

fn acquire_lock(intermediate_path: &std::path::Path) -> Result<std::fs::File, std::io::Error>
{
    let lock_path = intermediate_path.join("lock");
    // Open read+write (no append) so we can overwrite PID when we take the lock.
    let mut lockfile = std::fs::File::options().read(true).write(true).create(true).open(&lock_path)?;
    
    'retry: loop 
    {
        match lockfile.try_lock()
        {
            Ok(_) => 
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
                                write_lock_pid(&mut lockfile)?;
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
                            write_lock_pid(&mut lockfile)?;
                            break 'retry;
                        }

                        lockfile.unlock()?;
                        continue;
                    }
                }
            },
            Err(e) => 
            {
                match e
                {
                    TryLockError::WouldBlock => 
                    {
                        // Short base + per-process jitter: quick handoff when lock frees (head room) and staggered retries.
                        let jitter_ms = (std::process::id() % 85) as u64;
                        std::thread::sleep(std::time::Duration::from_millis(15 + jitter_ms));
                        continue;
                    },
                    TryLockError::Error(error) => return Err(error)
                }
            }
        }
    }

    println!("Lock acquired");
    return Ok(lockfile);
}

/// Ensure HeaderGenerated/Public and HeaderGenerated/Private exist so the build's include path is valid
/// and the header-parser has somewhere to write. Matches header-parser GetDestinationPath layout.
fn ensure_header_generated_dirs(intermediate_path: &std::path::Path) -> Result<(), std::io::Error>
{
    let public = intermediate_path.join("HeaderGenerated").join("Public");
    let private = intermediate_path.join("HeaderGenerated").join("Private");
    std::fs::create_dir_all(&public)?;
    std::fs::create_dir_all(&private)?;
    Ok(())
}

/// Copy the project's <Config>.dep (from Sharpmake) to Public/<Config>.dep and Private/<Config>.dep
/// under intermediate_path so the header-parser can find it for module dependency mapping.
/// If the source .dep does not exist (e.g. project has no deps or solution not regenerated), write an empty file
/// so the header-parser still runs and generates empty s_dependencies_ for ECLASS(module).
fn ensure_dep_files(intermediate_path: &std::path::Path, project_dir: &std::path::Path, configuration: &str) -> Result<(), Box<dyn std::error::Error>>
{
    let dep_name = format!("{}.dep", configuration);
    let source = project_dir.join(&dep_name);
    let content = std::fs::read_to_string(&source).unwrap_or_default();

    let public_dir = intermediate_path.join("Public");
    let private_dir = intermediate_path.join("Private");
    std::fs::create_dir_all(&public_dir)?;
    std::fs::create_dir_all(&private_dir)?;

    std::fs::write(public_dir.join(&dep_name), &content)?;
    std::fs::write(private_dir.join(&dep_name), &content)?;
    Ok(())
}

/// Copy and diff without holding the lock. Ensures per-project repo exists first so diff_git runs in this project only.
fn prepare_without_lock(intermediate_path: &std::path::Path, project_dir: &std::path::Path, git_dir: &std::path::Path, configuration: &str, paths_copied: &mut Vec<String>) -> Result<(), Box<dyn std::error::Error>>
{
    println!("Check git repository");
    check_git(&git_dir, &intermediate_path)?;
    ensure_header_generated_dirs(intermediate_path)?;
    ensure_dep_files(intermediate_path, project_dir, configuration)?;
    println!("Copy new headers");
    copy_headers(&intermediate_path, &project_dir, paths_copied)?;
    println!("Check diff with git");
    diff_git(&git_dir, &intermediate_path)?;
    Ok(())
}

/// Remove copied header files under intermediate_path (used when header-parser fails so next run retries).
fn remove_copied_headers(intermediate_path: &std::path::Path, paths_copied: &[String]) -> Result<(), std::io::Error>
{
    for rel in paths_copied
    {
        if rel.ends_with(".h")
        {
            let p = intermediate_path.join(rel);
            if p.exists()
            {
                let _ = std::fs::remove_file(&p);
            }
        }
    }
    Ok(())
}

/// Commit (add . in per-project repo), then write target file and run header parser.
/// Repo already inited by prepare_without_lock. No lock: each project has its own git repo.
/// On header-parser failure, removes copied headers so the next run can retry.
fn commit_and_run(engine_dir: &std::path::Path, git_dir: &std::path::Path, intermediate_path: &std::path::Path, configuration: &String, _project_name: &str, paths_copied: &[String]) -> Result<(), Box<dyn std::error::Error>>
{
    println!("Commit diff to git");
    commit_git(&git_dir, &intermediate_path, &[])?;

    if !target_files.lock()?.is_empty()
    {
        println!("Write target file");
        write_target_file(&intermediate_path)?;
        println!("Starts header parser");
        if let Err(e) = run_headerparser(&engine_dir, &intermediate_path, &configuration)
        {
            let _ = remove_copied_headers(intermediate_path, paths_copied);
            return Err(e.into());
        }
        let _ = commit_git(&git_dir, &intermediate_path, &[]);
    }
    Ok(())
}

fn collect_header_paths(dir: &std::path::Path, out: &mut Vec<std::path::PathBuf>) -> Result<(), std::io::Error> {
    for entry in std::fs::read_dir(dir)? {
        let path = entry?.path();
        if path.is_dir() {
            collect_header_paths(&path, out)?;
        } else if path.is_file() {
            let ext = path.extension().and_then(|e| e.to_str());
            if ext == Some("h") || ext == Some("hpp") {
                out.push(path);
            }
        }
    }
    Ok(())
}

/// Compute SHA256 of all .h/.hpp under project_dir (path|filehash sorted by path). Same logic as the old PS1 script.
fn compute_headers_hash(project_dir: &std::path::Path) -> Result<String, Box<dyn std::error::Error>>
{
    let mut paths = Vec::new();
    collect_header_paths(project_dir, &mut paths)?;
    paths.sort();

    let mut combined = Vec::new();
    for path in &paths {
        let bytes = std::fs::read(path)?;
        let mut hasher = Sha256::new();
        hasher.update(&bytes);
        let file_hash = hex::encode(hasher.finalize());
        let line = format!("{}|{}", path.display(), file_hash);
        combined.extend_from_slice(line.as_bytes());
    }

    let mut hasher = Sha256::new();
    hasher.update(&combined);
    Ok(hex::encode(hasher.finalize()))
}

fn main() 
{
    let args: Vec<String> = std::env::args().collect();

    if args.len() < 6
    {
        eprintln!("Usage: balius <EngineDir> <ProjectName> <SourceRoot> <GitDir> <ConfName>");
        return;
    }

    let engine_dir = std::path::Path::new(&args[1]);
    let project_name = &args[2];
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

    // Stagger start to avoid all header parser jobs firing at once
    let stagger_sec = (project_name.bytes().fold(0u64, |a, b| a.wrapping_add(b as u64)) % 20) as u64;
    if stagger_sec > 0 {
        std::thread::sleep(std::time::Duration::from_secs(stagger_sec));
    }

    let hash_dir = engine_dir.join("Intermediate").join("HeaderParser").join("Hash");
    let hash_file = hash_dir.join(format!("{}.hash", project_name));

    let current_hash = compute_headers_hash(project_dir).unwrap_or_default();
    if !current_hash.is_empty() {
        if let Ok(saved) = std::fs::read_to_string(&hash_file) {
            if saved.trim() == current_hash {
                println!("Header hash unchanged for {}, skipping", project_name);
                // Still ensure HeaderGenerated dirs exist so the folder is findable and build include path is valid.
                let intermediate_path = engine_dir.join("Intermediate").join("HeaderParser").join(project_name);
                let _ = std::fs::create_dir_all(&intermediate_path);
                let _ = ensure_header_generated_dirs(&intermediate_path);
                return;
            }
        }
    }

    // One git repo per project: no lock needed, no cross-project commits.
    let intermediate_path = engine_dir.join("Intermediate").join("HeaderParser").join(project_name);
    let _ = std::fs::create_dir_all(&intermediate_path);

    let mut paths_copied = Vec::new();
    if let Err(e) = prepare_without_lock(&intermediate_path, &project_dir, &git_dir, configuration, &mut paths_copied)
    {
        eprintln!("Prepare failed: {e}");
        return;
    }

    match commit_and_run(&engine_dir, &git_dir, &intermediate_path, &configuration, project_name, &paths_copied)
    {
        Ok(_) =>
        {
            if !current_hash.is_empty() {
                let _ = std::fs::create_dir_all(&hash_dir);
                let _ = std::fs::write(&hash_file, &current_hash);
            }
        },
        Err(e) =>
        {
            eprintln!("Unable to process the headers: {e}");
        }
    }
}
