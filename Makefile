all:
	cc -g breakpoints.c -o breakpoints
# -g adds debug symbols which we need for gdb

clean: 
	rm breakpoints