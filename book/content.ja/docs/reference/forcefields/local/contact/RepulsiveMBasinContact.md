+++
title = "RepulsiveMBasin"
weight = 700000
+++

# RepulsiveMBasinContactPotential

MultipleBasin力場で用いられる、[Go-Contact potential]({{<relref "GoContactPotential.md">}})の斥力項です。

{{<katex display>}}
U(r) = k\max\left(0, 5\left(\frac{r_0}{r}\right)^{12} - 6\left(\frac{r_0}{r}\right)^{10})
{{</katex>}}

## 例

```toml
[[forcefields.local]]
interaction = "Contact"
potential   = "RepulsiveMBasinContact"
topology    = "contact"
parameters = [
    {indices = [0, 1], v0 = 1.0, k = 0.1},
    # ...
]
```

## 入力


- `v0`: 浮動小数点数型
  - 最安定距離を指定します。
- `k`: 浮動小数点数型
  - パラメータの強さを指定します。
- `indices`: 整数の配列型（長さ: 2）
  - どの粒子の間に適用するかを指定します。最初の粒子は0番めです。
- `offset`: 整数型（省略可能）
  - インデックスに加算する値です。省略可能です。
