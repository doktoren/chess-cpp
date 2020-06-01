"""
Example:
python debugging/diff.py tables/KBKP_wtm.dat tables/maybeok/KBKP_wtm.dat
python debugging/diff.py tables/KNKP_wtm.dat tables/maybeok/KNKP_wtm.dat
python debugging/diff.py tables/KNK_wtm.dat tables/maybeok/KNK_wtm.dat
python debugging/diff.py tables/KPKP_wtm.dat tables/maybeok/KPKP_wtm.dat
python debugging/diff.py tables/KPK_wtm.dat tables/maybeok/KPK_wtm.dat
python debugging/diff.py tables/KQKB_wtm.dat tables/maybeok/KQKB_wtm.dat
python debugging/diff.py tables/KQKN_wtm.dat tables/maybeok/KQKN_wtm.dat
python debugging/diff.py tables/KQKP_btm.dat tables/maybeok/KQKP_btm.dat
python debugging/diff.py tables/KQKP_wtm.dat tables/maybeok/KQKP_wtm.dat
python debugging/diff.py tables/KQK_wtm.dat tables/maybeok/KQK_wtm.dat
python debugging/diff.py tables/KQPK_wtm.dat tables/maybeok/KQPK_wtm.dat
python debugging/diff.py tables/KRKP_wtm.dat tables/maybeok/KRKP_wtm.dat
"""

import sys

def main(fn1, fn2, max_count):
    with open(fn1, "rb") as f:
        data1 = f.read()
    with open(fn2, "rb") as f:
        data2 = f.read()
    for i, (b1, b2) in enumerate(zip(data1, data2)):
        if b1 != b2:
            print("{} != {} at index {}".format(ord(b1), ord(b2), i))
            max_count -= 1
            if not max_count:
                break

main(sys.argv[1], sys.argv[2], int(sys.argv[3] if len(sys.argv) > 3 else 10))
