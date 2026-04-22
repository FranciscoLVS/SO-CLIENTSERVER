#!/bin/bash

DOC_FOLDER="server"
KEYWORD="King" # Palavra-chave para busca
DEFAULT_CACHE=100 # Tamanho padrão da cache
ITERATIONS=5 # Número de iterações para cada teste
CLIENT_TIMEOUT="15s" # Timeout para cada chamada do dclient (e.g., 15 segundos)

# Limpeza de processos anteriores ao iniciar e ao sair
cleanup() {
    echo "A limpar processos antigos..."
    pkill -f "./dserver" 2>/dev/null || true # Limpa processos do servidor
    pkill -f "./dclient" 2>/dev/null || true # Limpa processos do cliente
}

trap cleanup EXIT # Executa cleanup ao sair do script
cleanup # Limpeza inicial

echo "A começar teste de desempenho..."

# -------------------------------------------------------
# 1. Teste de desempenho de busca paralela
# -------------------------------------------------------
echo "1. Teste de desempenho de busca paralela"
echo "Processos,Tempo Médio" > parallel_search_results.csv

# Função para medir tempo
measure_time() {
    local start end dur dur_calc_result cmd_status
    
    start=$(LC_ALL=C date +%s.%N)

    # Executa o comando com timeout e redireciona a saída para /dev/null
    timeout "$CLIENT_TIMEOUT" "$@" > /dev/null 2>&1
    cmd_status=$? # Captura o status de saída (124 para timeout)
    
    end=$(LC_ALL=C date +%s.%N)

    if [ "$cmd_status" -eq 124 ]; then
        dur="0.0" # Ou um valor alto para indicar falha, ex: 99999
    elif [ "$cmd_status" -ne 0 ]; then
        dur="0.0" # Comando falhou por outra razão
    else
        # Calcula a duração. bc -l define a escala.
        dur_calc_result=$(LC_ALL=C echo "$end - $start" | bc -l 2>/dev/null)

        if [ -n "$dur_calc_result" ]; then
            # Verifica se o resultado é um número válido (pode ser negativo se o tempo for muito curto e houver imprecisão)
            # Um teste simples é verificar se bc não retornou algo não numérico.
            # Para simplificar, assumimos que se não estiver vazio, é o tempo.
            # Um resultado negativo pode ser tratado como 0.
            if (( $(echo "$dur_calc_result < 0" | bc -l) )); then
                dur="0.0"
            else
                dur="$dur_calc_result"
            fi
        else
            # bc falhou ou produziu saída vazia.
            dur="0.0"
        fi
    fi
    echo "$dur"
}

# 1. Teste de busca paralela
for proc in 1 2 4 8 16 64 128 256; do
    echo "Testando com $proc processos..."
    ./dserver "$DOC_FOLDER" "$DEFAULT_CACHE" &
    SERVER_PID=$!
    sleep 3  # Aumentar um pouco o tempo para o servidor iniciar completamente

    total="0.0" # Inicializa total como string float para bc
    for i in $(seq 1 $ITERATIONS); do
        # Chama measure_time; a saída de dclient já é tratada dentro da função
        t=$(measure_time ./dclient -s "$KEYWORD" "$proc")
        
        # Verifica se o tempo retornado é um número válido
        # Se não for, define como 0.0
        # O sed remove caracteres não numéricos, exceto . e - para evitar erros de bc
        # O regex verifica se é um número válido (opcionalmente negativo e com ponto decimal)
        # O -z verifica se a string está vazia
        # O resultado é passado para bc para somar os tempos
        # O resultado final é uma string float, que é convertida para número no bc
        t_sanitized=$(echo "$t" | sed 's/[^0-9.-]//g') # Remove caracteres não numéricos exceto . e -
        if ! [[ "$t_sanitized" =~ ^-?[0-9]*(\.[0-9]+)?$ ]] || [ -z "$t_sanitized" ]; then
            t_sanitized="0.0"
        fi
        
        total=$(echo "$total + $t_sanitized" | bc -l)
    done
    
    avg=$(echo "scale=10; $total / $ITERATIONS" | bc -l) # Adiciona scale para a média
    echo "$proc,$avg" >> parallel_search_results.csv

    kill "$SERVER_PID" 2>/dev/null
    wait "$SERVER_PID" 2>/dev/null # Espera o processo do servidor terminar
    sleep 1
done

echo -e "\n2. Teste do impacto do tamanho da cache"
echo "CacheSize,Tempo Médio" > cache_results.csv

# 2. Teste do impacto do tamanho da cache
for cache_size_val in 10 50 100 200 1000 2048; do # Valores de cache para teste
    echo "Testando com tamanho de cache $cache_size_val..."
    ./dserver "$DOC_FOLDER" "$cache_size_val" &
    SERVER_PID=$!
    sleep 3 # Aumentar um pouco o tempo para o servidor iniciar

    total="0.0"
    for i in $(seq 1 $ITERATIONS); do
        t=$(measure_time ./dclient -s "$KEYWORD" 1) # Usando 1 processo para teste de cache 
        
        t_sanitized=$(echo "$t" | sed 's/[^0-9.-]//g')
        if ! [[ "$t_sanitized" =~ ^-?[0-9]*(\.[0-9]+)?$ ]] || [ -z "$t_sanitized" ]; then
            t_sanitized="0.0"
        fi

        total=$(echo "$total + $t_sanitized" | bc -l)
    done
    
    avg=$(echo "scale=10; $total / $ITERATIONS" | bc -l)
    echo "$cache_size_val,$avg" >> cache_results.csv

    kill "$SERVER_PID" 2>/dev/null
    wait "$SERVER_PID" 2>/dev/null
    sleep 1
done

echo "Testes concluídos. Verifique parallel_search_results.csv e cache_results.csv para os resultados."