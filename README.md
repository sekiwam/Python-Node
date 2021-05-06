
prev=$(git rev-list HEAD -n 1); git pull; test $prev = $(git rev-list HEAD -n 1) || git log