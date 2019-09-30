# 3SPN2BaseStackingInteraction

`3SPN2BaseStacking`相互作用は、3SPN2系力場で用いられるbase-stackingの力場です。

利用可能なポテンシャルは以下のものがあります。
- `"3SPN2"`
  - Hinckley et al., (2013) JCP
- `"3SPN2C"`
  - Freeman et al., (2014) JCP

## 例

```toml
[[forcefields.local]]
interaction = "3SPN2BaseStacking"
potential   = "3SPN2"
topology    = "nucleotide"
parameters  = [
# `nucleotide` index starts from 5' and ends at 3'.
{strand = 0, nucleotide =  0,          S =   0, B =   1, Base = "A"},
{strand = 0, nucleotide =  1, P =   2, S =   3, B =   4, Base = "T"},
{strand = 0, nucleotide =  2, P =   5, S =   6, B =   7, Base = "C"},
{strand = 0, nucleotide =  3, P =   8, S =   9, B =  10, Base = "G"},
{strand = 1, nucleotide =  4,          S =  11, B =  12, Base = "C"},
{strand = 1, nucleotide =  5, P =  13, S =  14, B =  15, Base = "G"},
{strand = 1, nucleotide =  6, P =  16, S =  17, B =  18, Base = "A"},
{strand = 1, nucleotide =  7, P =  19, S =  20, B =  21, Base = "T"},
]
```

## 入力

`BondLength`相互作用を用いる場合、`[[forcefields.local]]`テーブルには、以下の値を設定します。

- `interaction`: 文字列型
  - 相互作用の種類を設定します。
  - `"3SPN2BaseStacking"`を指定します。
- `potential`: 文字列型
  - ポテンシャルの種類を設定します。
  - `3SPN2` : 3SPN.2ポテンシャルのパラメータを使います。
  - `3SPN2C`: 3SPN.2Cポテンシャルのパラメータを使います。
- `topology`: 文字列型
  - [`"Topology"`](Topology.md)に設定する名前を指定します。
  - この力場は、隣り合うヌクレオチド間の全ての粒子対にエッジを張ります。
- `parameters`:テーブルの配列型
  - 力場が適用される粒子ごとにパラメータを与えます。ここでは、
    - ヌクレオチドが所属するストランドのインデックス
    - ヌクレオチドの通し番号
    - リン酸、糖、塩基に対応する粒子の番号、
    - 塩基の種類
  - を指定します。
  - 端のヌクレオチドには、多くの場合リン酸がありません。