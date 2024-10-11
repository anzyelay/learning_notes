# git

## Git全局设置

```sh
git config --global user.name "leixa"
git config --global user.email "leixa@jideos.com"
git config --global core.editor "vim" //修改commit的编辑器
git config --global alias.st status
# 设置github.com的镜像替换仓库加速访问
git config --global url."https://git.homegu.com".insteadOf https://github.com
```

## 仓库的建立

1. 从一个空仓库拉取创建

    ```bash
    git clone git@192.168.16.198:leixa/test.git
    cd test
    touch README.md
    git add README.md
    git commit -m "add README"
    git push -u origin master
    ```

1. 在本地现有文件夹创建仓库并推送

    ```bash
    cd existing_folder
    git init
    git remote add origin git@192.168.16.198:leixa/test.git
    git add .
    git commit -m "Initial commit"
    git push -u origin master
    ```

1. 推送现有的 Git 仓库

    ```bash
    cd existing_repo
    git remote rename origin old-origin
    git remote add origin git@192.168.16.198:leixa/test.git
    git push -u origin --all
    git push -u origin --tags
    ```

---

## commit

### 修改当前的提交

- `git commit --amend`

### 修改历史的提交

- `git rebase`, “-i”是interative的意思

    ```sh
    # 从HEAD版本开始往过去数3个版本(--interactive:弹出交互式的界面进行编辑合并)
    $ git rebase -i HEAD~3
    # 合并当前到指定版本号间的COMMIT（不包含此版本）
    $ git rebase -i [father_commitid] [current_commitid]
    # 撤销修改
    $ git rebase --abort
    # 完成修改后更新index继续rebase处理，直到结束所有rebase
    $ git rebase --continue
    ```

- 说明：

    ```sh
    # 命令:
    # p, pick <提交> = 使用提交
    # r, reword <提交> = 使用提交，但修改提交说明
    # e, edit <提交> = 使用提交，进入 shell 以便进行提交修补
    # s, squash <提交> = 使用提交，但融合到前一个提交
    # f, fixup <提交> = 类似于 "squash"，但丢弃提交说明日志
    # x, exec <命令> = 使用 shell 运行命令（此行剩余部分）
    # b, break = 在此处停止（使用 'git rebase --continue' 继续变基）
    # d, drop <提交> = 删除提交
    # l, label <label> = 为当前 HEAD 打上标记
    # t, reset <label> = 重置 HEAD 到该标记
    # m, merge [-C <commit> | -c <commit>] <label> [# <oneline>]
    # .       创建一个合并提交，并使用原始的合并提交说明（如果没有指定
    # .       原始提交，使用注释部分的 oneline 作为提交说明）。使用
    # .       -c <提交> 可以编辑提交说明。
    #
    # 可以对这些行重新排序，将从上至下执行。 (commit的排列也是按历史从上到下排列)
    # 如果您在这里删除一行，对应的提交将会丢失。
    # 然而，如果您删除全部内容，变基操作将会终止。
    # 注意空提交已被注释掉  
    ```

- 仅修改提交信息，比如修改author或时间

    ```sh
    # 1. 将要修改的commit前的pick改为edit,保存退出
    git rebase -i father_commit_id

    # 2. 修改信息
    git commit --amend --reset-author` 
    # or
    git commit --date="$(date -R)"
    # or
    git commit --date=`(date -R)`

    # 3. 继续,执行 step2 进行下一个修改 或 提示完成全部commit修改
    git rebase --continue
    ```

- 合并多个commit，保持历史简洁，`git rebase` ,合并步骤
    1. 查看 log 记录，使用git rebase -i 选择要合并的commit之后的一个commit
    1. 编辑要合并的版本信息，保存提交(多条合并会出现多次,可能会出现冲突)
    1. 修改注释信息后，保存提交(多条合并会出现多次)
    1. 推送远程仓库或合并到主干分支

### 应用其它分支的某个commit

- `git cherry-pick commits -e` : -e[dit] 是重新编辑commit

---

## 标签

1. 显示所有tag标签

    ```sh
    git tag [--list]
    ```

2. 删除标签

    ```sh
    git tag -d tagname
    ```

3. 删除远程tag

    ```sh
    git push origin :refs/tags/tagname
    ```

4. 删除所有tag

    ```sh
    #delete locale
    git tag | xargs git tag -d 
    #delete remote
    git tag | xargs git push origin :refs/tags/  
    ```

---

## 日志

> SYNOPSIS
> `git log [<options>] [<revision range>] [[--] <path>...]`  
> path前的"--"是用来与前面的options和revision避免混淆分隔用的，没分歧时可省略  
> revision raqnge: 中间用“..“分隔, eg:"old..new" ,默认”orig..HEAD“

1. 查看单独一个文件/文件侠的修改历史  

    ```sh
    git log path
    git log --follow path ## 包括path被改名前的记录.
    git log -p path ##可以显示每次提交的diff
    git show c5e69804bbd9725 filename ##只看某次提交中的某个文件变化，可以直接加上fileName
    ```

2. 输出检索的日志信息

    ```sh
    git log --grep=<pattern>
    ```

---

## blame -- 查询某行代码修改记录

## format-patch/am/apply -- 补丁操作

## QA

1. How to ignore a tracked file in git without deleting it?

    ```sh
    git rm --cached  path/to/file 
    git rm --cached -r path/to/folder
    ```

1. 在push时远程有更新，如何实现不合并远程的提交，而是将你当前的几个提交的基于远程最新的commit上做提交？

    ```sh
    # pull 默认情况下是 fetch + merge
    # 加了--rebase后， 优先换基而不是合并。incorporate changes by rebasing rather than merging
    git pull --rebase 
    ```

    - the repository current status

    ```mermaid
    graph LR

    a --> b{b} --remote fresh commit -->　c --> d --> e 
    b--local commit--> h-->j 
    ```

    - git pull

    ```mermaid
    graph LR
    a --> b{b} --remote fresh commit -->　c --> d --> e 
    b--local commit--> h-->j 
    e-.-> M((merge))
    j-..-> M --> m
    ```

    - git pull --rebase

    ```mermaid
    graph LR
    a --> b --remote commit -->　c --> d --> e 
    e --rebase---> h --> j
    b -. X .-> h
    ```

1. 应用其它分支的某个commit

    ```sh
    git cherry-pick commits -e # -e[dit] 是重新编辑commit
    ```

1. 删除本地存在但远程已删除的分支

   ```sh
   git remote prune origin
   ```
