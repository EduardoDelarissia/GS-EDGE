Projeto Desenvolvido por Eduardo Antonio Delarissia RM:563468


📌 Totem Motivacional Inteligente com ESP32 + FIWARE

Global Solution – FIAP – 2025

Este projeto apresenta um Totem Motivacional Inteligente, desenvolvido em ESP32 com integração ao FIWARE Orion Context Broker.
O sistema exibe frases motivacionais e orientações de descanso em um display TFT, alternando a cada 30 segundos.
As informações exibidas também são enviadas ao FIWARE, permitindo monitoramento remoto, auditoria e análise dos hábitos de motivação e pausas.

🚀 Funcionalidades
✅ 1. Exibição de frases motivacionais

Obtidas via API externa JSON:

https://motivacional.top/api.php?acao=aleatoria

✅ 2. Exibição de frases de descanso

Inclui 10 mensagens pré‐definidas para promover bem-estar.

✅ 3. Alternância automática

A cada 30 segundos, alterna:
Motivação → Descanso → Motivação → …

✅ 4. Display TFT ILI9341

Mostra textos com quebra de linha, cores e layout limpo.

✅ 5. WiFi Inteligente

Conexão automática

Reconexão em caso de queda

Timeout controlado para Wokwi

✅ 6. Integração FIWARE

A cada 30s o ESP32 envia ao Orion Context Broker:

última frase exibida

tipo (motivacional/descanso)

contador de envios

Isso permite que aplicações externas acompanhem o estado do totem em tempo real.

🏗️ Arquitetura da Solução
            ┌─────────────────────────┐
            │      API Motivacional    │
            │  https://motivacional... │
            └──────────────┬──────────┘
                           │ JSON
                           ▼
┌──────────────┐    WiFi     ┌──────────────────────┐
│   ESP32       │────────────▶│  FIWARE Orion CB      │
│  (Wokwi)      │             │  (VM Azure – Porta 1026) │
└──────┬───────┘             └───────────┬──────────┘
       │                 Upsert NGSIv2   │
       │                                  │
       ▼                                  ▼
┌──────────────┐               ┌──────────────────────┐
│  Display TFT  │               │ Postman / FIWARE GUI │
│ (Motivação)   │               │ Consulta da entidade │
└──────────────┘               └──────────────────────┘

🌐 Integração FIWARE – Detalhes Técnicos
✔️ Host (VM Azure)
20.63.110.194

✔️ Porta Orion
1026

✔️ Serviço
fiware-service: smart

✔️ Service Path
fiware-servicepath: /

✔️ Entidade usada
{
  "id": "FocusCoach01",
  "type": "BreakAssistant"
}

✔️ Requisição enviada pelo ESP32
POST http://20.63.110.194:1026/v2/entities?options=upsert


Com JSON:

{
  "id": "FocusCoach01",
  "type": "BreakAssistant",
  "lastPhrase": { "value": "Frase exibida", "type": "Text" },
  "phraseType": { "value": "motivacional", "type": "Text" },
  "sendCount": { "value": 12, "type": "Integer" }
}

🛠️ Como Executar no Wokwi
1. Acesse o projeto no simulador

🔗 https://wokwi.com/projects/448059420429065217

2. Abra o console (CTRL + SHIFT + K)
3. Verifique:

Conexão WiFi

Frases aparecendo

Envio FIWARE a cada 30s:

Exemplo esperado:

Enviando dados ao FIWARE...
HTTP FIWARE: 201

🧪 Como Testar no Postman
1. Criar/ler entidade

GET

http://20.63.110.194:1026/v2/entities/FocusCoach01


Headers obrigatórios:

fiware-service: smart
fiware-servicepath: /

2. Resposta esperada
{
  "id": "FocusCoach01",
  "type": "BreakAssistant",
  "lastPhrase": { "value": "Respire fundo...", "type": "Text" },
  "phraseType": { "value": "descanso", "type": "Text" },
  "sendCount": { "value": 27, "type": "Integer" }
}
