#!/bin/bash

export ANDROID_HOME=$HOME/android-sdk
export ANDROID_NDK_HOME=$HOME/android-sdk/ndk-bundle
export ANDROID_NDK_ROOT=$HOME/android-sdk/ndk-bundle

scons platform=android
