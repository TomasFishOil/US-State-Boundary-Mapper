# US-State-Boundary-Mapper

This program maps a U.S. state boundary into a BMP file for all coterminous states, outputing utilizing Windows Paint.

The "lower48BoundaryFile.txt" contains:
- Numeric state code for each state
- A count of the valid points which follow in the next lines
- A series of lines containing four floating point longitude-latitude pairs
  
~ Some implementation was done in inline x86 assembly for learning experience. ~
