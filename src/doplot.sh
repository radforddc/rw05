plot $1 -d
plot2ps $1 -d
ps2pdf $1.ps
open $1.pdf
rm $1.p[ds]g $1.ps
