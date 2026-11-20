set datafile separator ","

set terminal pngcairo size 1000,700 enhanced font 'Arial,12'
set output 'plot_linear_step.png'

set xlabel "tau" font 'Arial,12'
set ylabel "Y" font 'Arial,12'
set grid
set key outside right

set style line 1 linecolor rgb '#0060ad' linetype 1 linewidth 2 pointtype 7 pointsize 1.5

set title "Linear model - step" font 'Arial,14'

plot 'linear_step.csv' using 1:3 with linespoints linestyle 1 title "y(tau)"