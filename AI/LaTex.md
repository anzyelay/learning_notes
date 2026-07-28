# Markdown 公式语法（LaTeX）

---

## 一、基本结构

```markdown
# 行内公式
$公式$

# 独立行公式（居中显示）
$$公式$$
```

---

## 二、上下标

| 语法 | 效果 | 说明 |
|------|------|------|
| `x^2` | $x^2$ | 上标 |
| `x_i` | $x_i$ | 下标 |
| `x_i^2` | $x_i^2$ | 同时有上下标 |
| `x^{10}` | $x^{10}$ | 多字符上标需 `{}` |
| `x_{ij}` | $x_{ij}$ | 多字符下标需 `{}` |

---

## 三、分数

```markdown
\frac{分子}{分母}
```

| 语法 | 效果 |
|------|------|
| `\frac{a}{b}` | $\frac{a}{b}$ |
| `\frac{x+1}{x-1}` | $\frac{x+1}{x-1}$ |
| `\frac{1}{2}` | $\frac{1}{2}$ |

---

## 四、求和 / 求积 / 积分

| 语法 | 效果 |
|------|------|
| `\sum_{i=1}^{N} x_i` | $\sum_{i=1}^{N} x_i$ |
| `\prod_{i=1}^{N} x_i` | $\prod_{i=1}^{N} x_i$ |
| `\int_{0}^{1} f(x)dx` | $\int_{0}^{1} f(x)dx$ |
| `\iint f(x,y)dxdy` | $\iint f(x,y)dxdy$ |
| `\oint f(x)dx` | $\oint f(x)dx$ |

---

## 五、根号

| 语法 | 效果 |
|------|------|
| `\sqrt{x}` | $\sqrt{x}$ |
| `\sqrt[3]{x}` | $\sqrt[3]{x}$ |
| `\sqrt[n]{x}` | $\sqrt[n]{x}$ |

---

## 六、希腊字母

| 语法 | 效果 | 语法 | 效果 |
|------|------|------|------|
| `\alpha` | $\alpha$ | `\beta` | $\beta$ |
| `\gamma` | $\gamma$ | `\delta` | $\delta$ |
| `\theta` | $\theta$ | `\lambda` | $\lambda$ |
| `\mu` | $\mu$ | `\sigma` | $\sigma$ |
| `\pi` | $\pi$ | `\omega` | $\omega$ |
| `\Sigma` | $\Sigma$ | `\Omega` | $\Omega$ |

> 💡 首字母大写 = 大写希腊字母

---

## 七、运算符

| 语法 | 效果 | 语法 | 效果 |
|------|------|------|------|
| `\times` | $\times$ | `\div` | $\div$ |
| `\pm` | $\pm$ | `\mp` | $\mp$ |
| `\cdot` | $\cdot$ | `\ast` | $\ast$ |
| `\leq` | $\leq$ | `\geq` | $\geq$ |
| `\neq` | $\neq$ | `\approx` | $\approx$ |
| `\equiv` | $\equiv$ | `\propto` | $\propto$ |
| `\infty` | $\infty$ | `\partial` | $\partial$ |
| `\nabla` | $\nabla$ | `\forall` | $\forall$ |
| `\exists` | $\exists$ | `\in` | $\in$ |
| `\subset` | $\subset$ | `\cup` | $\cup$ |
| `\cap` | $\cap$ | `\emptyset` | $\emptyset$ |

---

## 八、箭头

| 语法 | 效果 |
|------|------|
| `\to` | $\to$ |
| `\rightarrow` | $\rightarrow$ |
| `\leftarrow` | $\leftarrow$ |
| `\Rightarrow` | $\Rightarrow$ |
| `\Leftrightarrow` | $\Leftrightarrow$ |
| `\uparrow` | $\uparrow$ |

---

## 九、括号自适应大小

```markdown
\left( \frac{a}{b} \right)
```

| 语法 | 效果 |
|------|------|
| `\left( x \right)` | $\left( x \right)$ |
| `\left[ x \right]` | $\left[ x \right]$ |
| `\left\{ x \right\}` | $\left\{ x \right\}$ |
| `\left| x \right|` | $\left| x \right|$ |

> 💡 用 `\left` 和 `\right` 括号会自动匹配内容高度。

---

## 十、函数名

```markdown
\sin, \cos, \tan, \log, \ln, \lim, \max, \min, \exp
```

| 语法 | 效果 |
|------|------|
| `\sin x` | $\sin x$ |
| `\log x` | $\log x$ |
| `\lim_{x \to 0} f(x)` | $\lim_{x \to 0} f(x)$ |
| `\max_{i} x_i` | $\max_{i} x_i$ |

---

## 十一、矩阵

```markdown
$$
\begin{pmatrix}
a & b \\
c & d
\end{pmatrix}
$$
```

$$\begin{pmatrix} a & b \\ c & d \end{pmatrix}$$

| 环境 | 括号类型 |
|------|----------|
| `pmatrix` | 圆括号 ( ) |
| `bmatrix` | 方括号 [ ] |
| `vmatrix` | 竖线 \| \| |
| `Bmatrix` | 花括号 { } |

---

## 十二、分段函数

```markdown
$$
f(x) = \begin{cases}
x^2 & x \geq 0 \\
-x^2 & x < 0
\end{cases}
$$
```

$$f(x) = \begin{cases} x^2 & x \geq 0 \\ -x^2 & x < 0 \end{cases}$$

---

## 十三、常用组合示例

```markdown
# 均值
$$\bar{x} = \frac{1}{N}\sum_{i=1}^{N} x_i$$

# 方差
$$\sigma^2 = \frac{1}{N}\sum_{i=1}^{N}(x_i - \bar{x})^2$$

# 导数
$$\frac{dy}{dx} = \lim_{\Delta x \to 0} \frac{\Delta y}{\Delta x}$$

# 向量
$$\vec{a} \cdot \vec{b} = |\vec{a}||\vec{b}|\cos\theta$$
```

1. 均值: $\bar{x} = \frac{1}{N}\sum_{i=1}^{N} x_i$

1. 方差: $\sigma^2 = \frac{1}{N}\sum_{i=1}^{N}(x_i - \bar{x})^2$

1. 导数: $\frac{dy}{dx} = \lim_{\Delta x \to 0} \frac{\Delta y}{\Delta x}$

1. 向量: $\vec{a} \cdot \vec{b} = |\vec{a}||\vec{b}|\cos\theta$

---

## 速记口诀

```markdown
^ 上标    _ 下标    {} 包裹多字符
\frac 分数   \sqrt 根号   \sum 求和
\left \right 自适应括号
```
