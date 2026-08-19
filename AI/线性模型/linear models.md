# Linear Models — Logistic Regression & Softmax Regression

📌 学习目标

目标|描述
-|-
理解线性模型的基本思想|y = w·x + b，参数学习
掌握逻辑回归 (Logistic Regression) | 二分类 + 交叉熵损失 + 梯度下降
掌握Softmax 回归 | 多分类推广
实现从零开始的 Python 代码 | NumPy + 手动求导
理解正则化 | L1/L2 防止过拟合

## 🧠 1. 背景动机

线性模型假设输入特征到输出之间存在线性关系：

$$y = \mathbf{w}^T \mathbf{x} + b$$

- $\mathbf{w}$: 权重向量
- $b$: 偏置项
- $\mathbf{x}$: 特征向量

---

## 🔍 2. 逻辑回归 (Logistic Regression)

2.1 二分类问题

- 输出 $y \in \{0, 1\}$
- 预测为条件概率 $P(y=1 \mid \mathbf{x})$

2.2 Sigmoid 函数

$$
\sigma(z) = \frac{1}{1 + e^{-z}}
$$

性质：

- 输出范围 $(0, 1)$
- 导数简洁：$\sigma'(z) = \sigma(z)(1 - \sigma(z))$

2.3 模型表达式
$$
P(y=1 \mid \mathbf{x}) = \sigma(\mathbf{w}^T \mathbf{x} + b)
$$

---

## ⚙️ 3. 损失函数：交叉熵 (Cross-Entropy)

$$
L(\mathbf{w}, b) = -\frac{1}{N} \sum_{i=1}^N \left[ y_i \log \hat{y}_i + (1 - y_i) \log (1 - \hat{y}_i) \right]
$$

📝 推导：最大似然估计 → 负对数似然 → 交叉ENTROPY 损失

---

## 📈 4. 参数优化：梯度下降 (Gradient Descent)

梯度下降法更新公式：
$$
\mathbf{w} \leftarrow \mathbf{w} - \eta \frac{\partial L}{\partial \mathbf{w}}
$$
对 $\mathbf{w}$ 的梯度为：
$$
\frac{\partial L}{\partial \mathbf{w}} = \frac{1}{N} X^T (\hat{Y} - Y)
$$

## 5. LogisticRegression Python实现

[参考](./LogisticRegression.py)

## 🧩 6. Softmax 回归 (Multiclass Extension)

用于多分类问题，输出为概率分布：
$$
P(y=k|\mathbf{x}) = \frac{e^{\mathbf{w}_k^T \mathbf{x}}}{\sum_{j} e^{\mathbf{w}_j^T \mathbf{x}}}
$$

- 参数矩阵 $\mathbf{W} \in \mathbb{R}^{D \times K}$

- 使用 多类交叉熵 作为损购函数

## 🔁 7. 正则化 (Regularization)

防止过拟合：

$$
L_{reg} = L + \lambda R(\mathbf{w})
$$

- L2 正则化：$R(\mathbf{w}) = \frac{1}{2}\|\mathbf{w}\|^2$
- L1 正则化：$R(\mathbf{w}) = \sum |\mathbf{w}|$ → 稀疏性

C 语言视角理解：正则化 = 添加惩罚项到损失函数中，让权重变小


1.1 模型定义
$$
P(y=k|\mathbf{x}) = \frac{e^{\mathbf{w}_k^T \mathbf{x}}}{\sum_{j=1}^{K} e^{\mathbf{w}_j^T \mathbf{x}}}
$$
- $\mathbf{W} \in \mathbb{R}^{D \times K}$ — 权重矩阵
- 每列 $\mathbf{w}_k$ 对应类别 $k$

1.2 损失函数
$$
L = -\frac{1}{N} \sum_{i=1}^N \sum_{k=1}^K y_{i,k} \log \hat{y}_{i,k}
$$
1.3 梯度推导
$$
\frac{\partial L}{\partial \mathbf{W}} = \frac{1}{N} X^T (\hat{Y} - Y)
$$
