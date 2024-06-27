#!/bin/bash

# Will generate the necessary header files using the extension API schema file
# as the provided ones do not include all needed definitions.
cd godot-cpp
scons platform=linux custom_api_file=gdextension/extension_api.json
cd ..