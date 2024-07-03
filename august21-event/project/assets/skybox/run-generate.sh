#!/bin/bash
# This convenience script automatically sets up a venv and installs needed dependencies for
# the generate script, useful on linux distros where using pip directly is disallowed.
venv_dir=".venv"

if [ ! -d "$venv_dir" ]; then
    python3 -m venv "$venv_dir"
fi

source "$venv_dir/bin/activate"

pip install -r requirements.txt
python generate.py
