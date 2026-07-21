## env prepare

建议外加一个虚拟环境（和 C++ 的 venv 类似，隔离项目依赖）：

```shell
python -m venv ai-env                # 创建虚拟环境
source ai-env/bin/activate           # 激活 (Linux/Mac)
# ai-env\Scripts\activate            # Windows 用这句
pip install numpy matplotlib jupyter # 装包
```

在vscode中配置

`shift + ctrl + p`
    > python: 创建环境 --> python -m venv xxx
    > python: 选择解释器 --> source xxx/bin/activate
