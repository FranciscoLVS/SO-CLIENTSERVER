#!/bin/bash
# Script to add document metadata from the Gcatalog file using dclient.
# Usage: ./addGdatasetMetadata.sh <Gcatalog_file>

# Check if exactly one argument (the input file) is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <Gcatalog_file> "
    exit 1
fi

INPUT_FILE="$1"
SERVER_DIR="server" # Diretoria onde estão os documentos

# Check if input file exists before proceeding
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: File '$INPUT_FILE' not found."
    exit 1
fi

# Initialize a counter for processing documents
COUNT=0

# Read the input file line by line, using tab ('\t') as a delimiter
# The first line (header) is skipped
while IFS=$'\t' read -r filename title year authors; do
    COUNT=$((COUNT + 1))
    
    # Adiciona o prefixo "server/" se não existir no caminho
    if [[ "$filename" != "$SERVER_DIR/"* ]]; then
        # Extrair apenas o nome do arquivo (sem diretórios)
        base_filename=$(basename "$filename")
        full_path="$SERVER_DIR/$base_filename"
    else
        full_path="$filename"
    fi

    # Print document metadata being processed
    echo "------------------------"
    echo "Filename: $full_path"
    echo "Title: $title"
    echo "Year: $year"
    echo "Authors: $authors"

    # Chama o cliente com os parâmetros corretamente formatados
    ./dclient -a "$title" "$authors" "$year" "$full_path"

done < <(tail -n +2 "$INPUT_FILE")

echo -e "\nAdded metadata for $COUNT files."