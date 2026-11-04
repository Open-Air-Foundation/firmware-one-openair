import subprocess


Import("env")

project_dir = env.subst("$PROJECT_DIR")
git_version = subprocess.check_output(
    ["git", "describe", "--tags", "--always", "--dirty"],
    cwd=project_dir,
    text=True,
).strip()

env.Append(CPPDEFINES=[("GIT_VERSION", env.StringifyMacro(git_version))])
