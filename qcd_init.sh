#!/bin/bash
# This file is part of qcd(1).
#
# Redefine the cd command to invoke qcd, and feed the output back to
#  the built-in cd 
cd()
  {
  CD=`qcd "$@"`
  builtin cd "$CD" 
  }
