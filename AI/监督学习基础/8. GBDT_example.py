import numpy as np

# 基学习器：简单的 CART 回归树（仅支持单特征分裂）
# 损失函数：平方损失（负梯度 = 残差）
# 框架：串行拟合负梯度 + 学习率收缩

class SimpleCARTRegressor:
    """极简 CART 回归树（用于 GBDT 基学习器）"""
    def __init__(self, max_depth=3):
        self.max_depth = max_depth
        self.tree = None

    def _mse(self, y):
        if len(y) == 0:
            return 0
        return np.mean((y - np.mean(y))**2)

    def _best_split(self, X, y):

        best_gain, best_feat, best_thresh = 0, None, None

        parent_mse = self._mse(y)
        n = len(y)

        for feat in range(X.shape[1]):
            thresholds = np.unique(X[:, feat])
            for thresh in thresholds:
                left_mask = X[:, feat] <= thresh
                right_mask = ~left_mask

                if left_mask.sum() == 0 or right_mask.sum() == 0:
                    continue

                # $\Delta = MSE_{parent} - (\frac {n_L}{n} MSE_L + \frac {n_R}{n} MSE_R)$
                gain = parent_mse - (
                    left_mask.sum() / n * self._mse(y[left_mask]) +
                    right_mask.sum() / n * self._mse(y[right_mask])
                )

                if gain > best_gain:
                    best_gain, best_feat, best_thresh = gain, feat, thresh

        return best_feat, best_thresh, best_gain

    def _build_tree(self, X, y, depth):

        if depth >= self.max_depth or len(y) <= 1 or self._mse(y) < 1e-10:
            return {"leaf": True, "value": np.mean(y)}

        feat, thresh, gain = self._best_split(X, y)

        if feat is None or gain < 1e-10:
            return {"leaf": True, "value": np.mean(y)}

        left_mask = X[:, feat] <= thresh

        return {
            "leaf": False,
            "feat": feat,
            "thresh": thresh,
            "left": self._build_tree(X[left_mask], y[left_mask], depth + 1),
            "right": self._build_tree(X[~left_mask], y[~left_mask], depth + 1),
        }

    def fit(self, X, y):
        self.tree = self._build_tree(X, y, 0)

    def _prefict_one(self, x, node):
        if node["leaf"]:
            return node["value"]
        if x[node["feat"]] <= node["thresh"]:
            return self._prefict_one(x, node["left"])
        return self._prefict_one(x, node["right"])

    def predict(self, X):
        return np.array([self._prefict_one(x, self.tree) for x in X])


class GBDTRegressor:
    """GBDT Regressor"""

    def __init__(self, n_estimator=100, learning_rate=0.1, max_depth=3):
        self.n_estimator = n_estimator
        self.learing_rate = learning_rate
        self.max_depth = max_depth
        self.trees = []
        self.init_pred = None

    def fit(self, X, y):
        # Step 1 : 初始化（均值）
        self.init_pred = np.mean(y)
        current_pred = np.full(len(y), self.init_pred)
        self.trees = []

        # Step 2: 迭代拟合负梯度
        for _ in range(self.n_estimator):
            # 负梯度 = 残差（平方损失下）
            residuals = y - current_pred

            # 拟合一棵新树
            tree = SimpleCARTRegressor(max_depth=self.max_depth)
            tree.fit(X, residuals)

            # 更新预测
            update = tree.predict(X)
            current_pred += self.learing_rate * update
            self.trees.append(tree)

    def predict_proba(self, X):
        f = np.full(X.shape[0], self.init_pred)

        for tree in self.trees:
            f +=  self.learing_rate * tree.predict(X)

        prob = 1 / (1 + np.exp(-f))
        return np.column_stack([1 - prob, prob])

# 扩展到分类：关键改动
# GBDT 做分类时，树仍然拟合的是连续值（负梯度），而不是类别标签。
# GBDT 分类器中的每一棵树都是回归树，拟合的是概率空间的梯度，最终通过 sigmoid 映射回概率。
# 以二分类 Logistic 损失为例：
class GBDTClassifier:
    """GBDT Classifer"""

    def __init__(self, n_estimator=100, learning_rate=0.1, max_depth=3):
        self.n_estimator = n_estimator
        self.learing_rate = learning_rate
        self.max_depth = max_depth
        self.trees = []
        self.init_pred = None

    def fit(self, X, y):
        # Step 1 : 初始化（均值）
        self.init_pred = np.log(np.mean(y == 1)/np.mean( y == -1))
        current_f = np.full(len(y), self.init_pred)
        self.trees = []

        # Step 2: 迭代拟合负梯度
        for _ in range(self.n_estimator):

            p = 1 / (1 + np.exp(-current_f)) # sigmoid
            residuals = y - p # 负梯度

            # 拟合一棵新树
            tree = SimpleCARTRegressor(max_depth=self.max_depth)
            tree.fit(X, residuals)

            # 更新预测
            update = tree.predict(X)
            current_f += self.learing_rate * update
            self.trees.append(tree)

    def predict(self, X):
        pred = np.full(X.shape[0], self.init_pred)

        for tree in self.trees:
            pred +=  self.learing_rate * tree.predict(X)

        return pred

if __name__ == "__main__":
    np.random.seed(42)
    X = np.random.randn(200, 3)
    y = 2 * X[:, 0] + np.sin(X[:, 1]) + 0.5 * X[:, 2] ** 2 + np.random.randn(200) * 0.1

    model = GBDTRegressor(n_estimator=50, learning_rate=0.1, max_depth=4)

    model.fit(X, y)
    pred = model.predict(X)
    mse = np.mean((y - pred) ** 2)
    print(f"Training MSE: {mse:.6f}")


