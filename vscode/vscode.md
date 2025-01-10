# vscode 使用说明

## vscode中的预定义变量

### [Variables Reference](https://code.visualstudio.com/docs/editor/variables-reference)

Predefined variables
The following predefined variables are supported:

- **${userHome}** - the path of the user's home folder
- **${workspaceFolder}** - the path of the folder opened in VS Code
- **${workspaceFolderBasename}** - the name of the folder opened in VS Code without any slashes (/)
- **${file}** - the current opened file
- **${fileWorkspaceFolder}** - the current opened file's workspace folder
- **${relativeFile}** - the current opened file relative to workspaceFolder
- **${relativeFileDirname}** - the current opened file's dirname relative to workspaceFolder
- **${fileBasename}** - the current opened file's basename
- **${fileBasenameNoExtension}** - the current opened file's basename with no file extension
- **${fileExtname}** - the current opened file's extension
- **${fileDirname}** - the current opened file's folder path
- **${fileDirnameBasename}** - the current opened file's folder name
- **${cwd}** - the task runner's current working directory upon the startup of VS Code
- **${lineNumber}** - the current selected line number in the active file
- **${selectedText}** - the current selected text in the active file
- **${execPath}** - the path to the running VS Code executable
- **${defaultBuildTask}** - the name of the default build task
- **${pathSeparator}** - the character used by the operating system to separate components in file paths
- **\${/}** - shorthand for **${pathSeparator}**

### Predefined variables examples

Supposing that you have the following requirements:

1. A file located at `/home/your-username/your-project/folder/file.ext` opened in your editor;
1. The directory `/home/your-username/your-project` opened as your root workspace.
So you will have the following values for each variable:

- **${userHome}** - `/home/your-username`
- **${workspaceFolder}** - `/home/your-username/your-project`
- **${workspaceFolderBasename}** - `your-project`
- **${file}** - `/home/your-username/your-project/folder/file.ext`
- **${fileWorkspaceFolder}** - `/home/your-username/your-project`
- **${relativeFile}** - `folder/file.ext`
- **${relativeFileDirname}** - `folder`
- **${fileBasename}** - `file.ext`
- **${fileBasenameNoExtension}** - `file`
- **${fileDirname}** - `/home/your-username/your-project/folder`
- **${fileExtname}** - `.ext`
- **${lineNumber}** - line number of the cursor
- **${selectedText}** - text selected in your code editor
- **${execPath}** - location of Code.exe
- **${pathSeparator}** - / on macOS or linux, \ on Windows

### 如何打印变量值？

创建一个task任务，打开终端运行任务即可

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "echo",
      "type": "shell",
      "command": "echo ${workspaceFolder}"
    }
  ]
}
```

## vscode remote debug

[参考blog](../blog/dbg-vscode-adb-gdb-wsl-debug.md)

1. vscode配置对应连接,配置**miDebuggerPath**,**miDebuggerServerAddress**

    ```json
    "miDebuggerServerAddress": "remote_ip:port"
    ```

## 配置项

| 配置项 | 说明 |
| :----: | :----: |
| vimvim.foldfix | 移动鼠标不自动展开折叠代码, enable它 |
