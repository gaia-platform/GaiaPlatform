This folder contains git-hooks. git-hooks are scripts that are triggered by common 
git commands (commit, push, etc..). git-hooks are stored in `.git/hooks`.

git-hooks are local to any git repo and are not copied over when you clone a repo.
We want to have consistent hooks across the team, for this reason we keep them 
here (part of the repository) and copy them into `.git/hooks` when CMake is invoked.
