# source: start1.s --march=v0_v10
# source: move-1.s --march=v32
# ld:
# error: contains CRIS v32 code

# Test that linking a v32 object to a (classic) v10 object does
# not work.
