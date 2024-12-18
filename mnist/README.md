# MNIST Benchmark

Using the Gravity compiler to build a handwritten digits recognizer from the
MNIST database [http://yann.lecun.com/exdb/mnist/]. The implementation uses
Gravity as a just-in-time compiler. A python3 reference implementation is
provided using tensorflow and identical model/hyper-parameters.

## Obtaining, Building & Running Gravity/MNIST
  1. $ git clone https://github.com/givargis/gravity
  2. $ cd gravity/src
  3. $ make
  4. $ cd ../mnist
  5. $ make
  6. $ ./mnist
  7. $ python3 mnist.py

## Performance

### iMac (Retina 5K, 27-inch, Late 2015)

Benchmarks running on:
  * Train/Test data preloaded in RAM and excluded from time measurements
  * 4 GHz Quad-Core Intel Core i7
  * 32 GB 1867 MHz DDR3
  * CentOS Linux release 8.0.1905 (Core)
  * GCC 8.2.1
  * Python 3.6.8
  * Tensorflow 2.0.0

```
   Implementation  | Accuracy |  Train Time   |   Test Time   | Mem. Train/Act.
                   |          | (usec/sample) | (usec/sample) |      (MB)
-------------------|----------|---------------|---------------|----------------
     Gravity/C     |  0.9670  |     124       |      18       |  0.722 / 0.358
-------------------|----------|---------------|---------------|----------------
 Python/Tensorflow |  0.9707  |     154       |      39       |  209* / ----

* measuring libtensorflow_framework.so.2 resident memory only
```

### Raspberry Pi 3 Model B+

Benchmarks running on:
  * Train/Test data preloaded in RAM and excluded from time measurements
  * 1.4 GHz 64-bit Quad-Core ARM Cortex-A53
  * 1 GB LPDDR2 SDRAM
  * Raspbian 9.11
  * GCC 6.3.0
  * Python 3.5.3
  * Tensorflow 1.14.0

```
   Implementation  | Accuracy |  Train Time   |   Test Time   | Mem. Train/Act.
                   |          | (usec/sample) | (usec/sample) |      (MB)
-------------------|----------|---------------|---------------|----------------
     Gravity/C     |  0.9670  |     2060      |      139      |  0.722 / 0.358
-------------------|----------|---------------|---------------|----------------
 Python/Tensorflow |  FAIL*   |     FAIL*     |     FAIL*     |  FAIL* / ----

* out of memory and unable to complete benchmark
```

### Raspberry Pi 4 Model B 2019 Quad Core 64

Benchmarks running on:
  * Train/Test data preloaded in RAM and excluded from time measurements
  * 1.5 GHz 64-bit Quad-Core ARM Cortex-A72
  * 4 GB LPDDR2 SDRAM
  * Raspbian GNU/Linux 10 (buster)
  * GCC 8.3.0
  * Python 3.7.3
  * Tensorflow 1.14.0

```
   Implementation  | Accuracy |  Train Time   |   Test Time   | Mem. Train/Act.
                   |          | (usec/sample) | (usec/sample) |      (MB)
-------------------|----------|---------------|---------------|----------------
     Gravity/C     |  0.9670  |     475       |       33      |  0.722 / 0.358
-------------------|----------|---------------|---------------|----------------
 Python/Tensorflow |  0.9707  |     644       |      111      |  209* / ----

* measuring libtensorflow_framework.so.2 resident memory only
```
