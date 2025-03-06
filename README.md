**techs**: C & SQLite

## prerequisite
- gcc 14.2.0

## build
```
git clone https://github.com/reihanboo/simple-todo.git
cd simple-todo
make
```

## usage
- `.\build\td.exe add "task description"`, does what it says
- `.\build\td.exe list`, lists tasks that isn't done
- `.\build\td.exe done task_id`, should list first
- `.\build\td.exe archive`, lists tasks that is both done and not done
