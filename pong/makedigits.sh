rm -f /tmp/d_*.png
for i in 0 1 2 3 4 5 6 7 8 9; do
	magick -size 50x100 xc:black -fill white -pointsize 70 -gravity center \
		-font JetBrainsMono-NF-Regular -annotate +0+0 "$i" /tmp/d_$i.png
done
magick /tmp/d_0.png /tmp/d_1.png /tmp/d_2.png /tmp/d_3.png /tmp/d_4.png \
	/tmp/d_5.png /tmp/d_6.png /tmp/d_7.png /tmp/d_8.png /tmp/d_9.png \
	+append textures/digits.png
magick identify textures/digits.png
