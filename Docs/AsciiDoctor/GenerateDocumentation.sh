#!/bin/bash

asciidoctor V-EZ.txt

sed '/<\/head>/i <style>code { font-size: 0.8em } #header, #content, #footer, #footnotes { max-width: 90% }<\/style>' V-EZ.html > temp.html
mv temp.html ../V-EZ.html