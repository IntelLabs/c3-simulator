#!/bin/bash
# Invoke this script from simics/internal/spec

pytest -n20 -v scripts -k test_spec_local --spec_year 17
