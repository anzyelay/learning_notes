import numpy as np

class LogisticRegression:
    def __init__(self, lr=0.01, n_iters=1000):
        self.lr = lr
        self.n_iters = n_iters
        self.weights = None
        self.bias = None

    def _prefict(self, X):
        z = np.dot(X, self.weights) + self.bias
        return self._sigmoid(z)

    def fit(self, X, y):
        n_samples, n_features = X.shape()
        self.weights = np.zeros(n_features)
        self.bias = 0

        for _ in range(self.n_iters):
            y_pred = self._predict(X)
            dy = y_pred - y

            dw = np.dot(X.T, dy) / n_samples
            db = np.sum(dy) / n_samples

            self.weights -= dw * self.lr
            self.bias -= db * self.lr

    def predict(self, X):
        y_pred = self._prefict(X)
        return (y_pred >= 0.5).astype(int)

    def _sigmoid(self, x):
        return 1 / (1 + np.exp(-x))
