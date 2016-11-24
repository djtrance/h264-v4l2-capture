all: capture
capture: capture.c V4L.c
	arm-linux-gcc -O2 -Wall  $^ -o $@

clean:
	rm -f capture
