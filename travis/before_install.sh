#!/usr/bin/env bash

if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    brew update
    brew tap homebrew/versions
    brew install glfw3
fi


