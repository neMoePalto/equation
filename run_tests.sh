#!/bin/bash

readonly PATH_TO_BIN='./build/debug/equation'

equations=($(awk '{print $1}' test_data.txt))
answers=(  $(awk '{print $2}' test_data.txt))

for ((i = 0; i < ${#equations[*]}; i++ )); do
  output=($($PATH_TO_BIN ${equations[i]}));

  last=${#output[*]}-1;
  res="";
  if [ ${answers[i]} == ${output[last]} ]; then
    res="OK";
  else
    res="FAILED, expected ";
    res="$res${answers[i]}";
  fi

  echo -e ${equations[i]} " \t -> x =" ${output[last]} $res;

  if [ "$res" != "OK" ]; then
    echo -e $($PATH_TO_BIN ${equations[i]}) ' \n ';
  fi

done
