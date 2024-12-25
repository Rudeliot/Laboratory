import numpy as np
import time

# Обычное произведение матриц
def classic_multiplication(A, B):
    n = A.shape[0]
    C = np.zeros((n, n))
    for i in range(n):
        for j in range(n):
            for k in range(n):
                C[i, j] += A[i, k] * B[k, j]
    return C

# Блочное произведение матриц
def block_multiplication(A, B, block_size):
    n = A.shape[0]
    C = np.zeros((n, n))
    for i in range(0, n, block_size):
        for j in range(0, n, block_size):
            for k in range(0, n, block_size):
                # Умножение блоков
                C[i:i+block_size, j:j+block_size] += np.dot(
                    A[i:i+block_size, k:k+block_size],
                    B[k:k+block_size, j:j+block_size]
                )
    return C

# Замеры времени
def measure_time(func, *args, **kwargs):
    start_time = time.time()
    result = func(*args, **kwargs)
    return result, time.time() - start_time

# Тестирование
n = 128  # Размер матриц
block_size = 64  # Размер блока

# Генерация случайных матриц
A = np.random.rand(n, n)
B = np.random.rand(n, n)

# Обычное умножение
C_classic, classic_time = measure_time(classic_multiplication, A, B)
print(f"\nВремя обычного умножения: {classic_time:.4f} сек")


# Вывод фрагментов матриц (первые 5x5 элементов для наглядности)
print("\nФрагмент результата (обычное умножение):")
print(C_classic[:5, :5])

# Блочное умножение
C_block, block_time = measure_time(block_multiplication, A, B, block_size)
print(f"\nВремя блочного умножения: {block_time:.4f} сек") 

print("\nФрагмент результата (блочное умножение):")
print(C_block[:5, :5])

