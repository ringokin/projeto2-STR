
# Célula de Manufatura

Este repositório é referente ao Projeto 02 da disciplina Sistemas em Tempo Real, no período 2024.2, pela Universidade Federal de Campina Grande (UFCG).

---
## Equipe

- Jayne Emilly Silva de Melo (121210548)
- Victor Hugo Melquíades Klein (119110066)
  
---
  
# Instruções
- abrir 'FreeRTOS\projeto\main\WIN32.sln' com o Visual Studio 2022  <br />
- compilar e executar

---

## Descrição do Projeto 

Este projeto tem como objetivo simular uma célula de manufatura utilizando o FreeRTOS, onde máquinas e robôs interagem em um processo de produção. A célula consiste em três máquinas de produção (M1, M2 e M3), quatro robôs (R1, R2, R3 e R4) e depósitos que controlam os insumos e produtos. O sistema simula a movimentação dos robôs entre os depósitos e as máquinas, respeitando regras de tempo de movimentação e produção.

---

## Objetivos do Projeto
1. Desenvolver um sistema de manufatura utilizando o FreeRTOS.

2. Simular a interação entre máquinas, robôs e depósitos em um processo contínuo de produção.

3. Avaliar se ocorrem paradas nas máquinas e propor soluções para evitar congestionamentos.

4. Implementar o controle de tempo de movimentação dos robôs, de modo a otimizar o fluxo da produção.

---

## Funcionalidades

O sistema desenvolvido simula a operação de uma célula de manufatura com as seguintes funcionalidades:

Controle de máquinas: M1, M2 e M3 produzem itens conforme o tempo de produção determinado.

Movimentação de robôs: Quatro robôs realizam o transporte de itens entre os depósitos e máquinas, com tempos específicos de movimentação.

Gerenciamento de depósitos: Cada máquina e a célula possuem depósitos que armazenam itens antes e depois do processo de produção.

Paradas e otimização: O sistema verifica se alguma máquina para devido à falta de itens ou capacidade de armazenamento e propõe ajustes no tempo de movimentação dos robôs para resolver o problema.

--- 

## Modelagem do Sistema
A célula de manufatura é composta por:

Máquinas:

- M1 e M2 produzem um item a cada 1,5 segundos.

- M3 produz um item a cada 3 segundos.

Robôs:

- R1 retira itens do depósito de entrada da célula e coloca no depósito de entrada de M1.

- R2 retira itens de M1 e coloca no depósito de entrada de M2.

- R3 retira itens de M1 e coloca no depósito de entrada de M3.

- R4 retira itens produzidos por M2 e M3 e coloca no depósito de saída da célula.

Depósitos:

- Cada máquina tem um depósito de entrada e saída.

- O depósito de entrada da célula armazena insumos para as máquinas.

- O depósito de saída da célula recebe os itens produzidos pelas máquinas.

Tempos de Operação
Tempo de movimentação dos robôs:

- R1, R2, e R4: 0,5 segundo por trajeto.

- R3: 0,8 segundo por trajeto.

Tempo de produção das máquinas:

- M1 e M2: 1,5 segundos por item.

- M3: 3 segundos por item.

Tempo de interação com os depósitos: 0,1 segundo por operação de retirar ou colocar um item.

Condições Especiais:
O depósito de cada máquina só pode armazenar um item por vez.

As máquinas param se não houver espaço disponível na saída ou se não houver itens para processamento.

--- 

## Estrutura do Projeto

--- 

## Exemplo de Execução
![Imagem de exemplo](projeto2-STR/ExemplodeExecução.png)




--- 

## Instruções de Instalação e Execução

---

## Considerações Finais

---

## Vídeo de apresentação
Link de acesso ao vídeo no YouTube:
