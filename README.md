# sintab

A quick sine table test I did for.. something else :)

![[](screenie.jpg)](screenie.jpg)

## description

This is an implementation of a table-based `sin` (and `cos`) replacement, useful for example in software synthesizers. It's not guaranteed to be the fastest around, but gained some quite noticeable speed gains in a certain sine-heavy project I have :) .

The table used is a 1024-element table of `doubles` representing the value of `sin` applied to the range `[0, PI)`. This computes the first (positive) half of the `sin` cycle; the second half is its mirror about the y-axis. To implement `fastSin` for any value `x`, we remap `x` to a fixed-point binary number with a certain number of whole and fractional bits using a single FP multiply and a cast. Using integer bitmasks and shifts, we then isolate the parts of this number representing the whole component (used to index the table) and the fractional component (used to linearly interpolate between two entries in the table). We then compute the final sample using a couple lookups, multiplies, and adds, and finally, depending on a sign bit from the remapped integer `x`, optionally invert the result's sign. `fastCos` just calls `fastSin` with an offset of `PI / 2`.

As a test, I also tested just storing the range `[0, PI/2)` in the table, as all 4 quadrants of a `sin` function can be reproduced from only this using mirroring/inverting. However, I found that the resulting reduction in table size didn't have any affect on performance on my machine at least (and 8kb for the current table is a pretty good size already). In fact, the extra logic for mirroring the curve about the x-axis as well as the inverting we already do about the y-axis made the function perform noticeably slower. This was only by a few percent, but with no noticeable speed increase from the reduced table size, wasn't very appealing. Additionally, it more than quadrupled our average error (which is already quite low, but didn't seem like it was worth it given we didn't gain any noticeable speed increase from the change). So, I chose to stick with the `[0, PI)` range in the current implementation.

Really though, I just like making isolated test projects for stuff like this every once in awhile :)

## license

This code is licensed under the MIT license (see LICENSE).
