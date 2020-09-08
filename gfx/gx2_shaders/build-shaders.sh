#!/bin/bash
set -e

latte-assembler compile --vsh $1.vsh --psh $1.psh $2.gsh
xxd -i $2.gsh | sed -E "s/\w+\[\]/${3}_shader[]/g" > $2
