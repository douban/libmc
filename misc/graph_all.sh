#!/bin/bash

gen_graph () {
  gprof2dot --format=callgrind --output=$2 $1
  dot -Tpng $2 -o $3

  echo Gen graph $3
}

for i in `ls callgrind.out.*`; do
  seq=${i##callgrind.out.}
  dot_filename=out.dot.$seq
  graph_filename=graph.$seq.png
  if ! [[ -f $graph_filename ]]; then
    gen_graph $i $dot_filename $graph_filename
  fi
done
