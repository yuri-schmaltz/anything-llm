# Instalador gráfico AnythingLLM (Qt 6)

Este diretório contém um instalador gráfico desenvolvido com Qt 6 para facilitar a distribuição do AnythingLLM.

## Funcionalidades

* **Verificação inteligente da instalação**: identifica automaticamente instalações prévias, informando a versão encontrada e oferecendo ações de atualização ou reparo quando necessário.
* **Instalação guiada**: permite escolher o diretório de destino e acompanha o progresso da cópia dos arquivos da aplicação.
* **Criação de atalhos**: gera atalhos para a aplicação tanto na área de trabalho quanto no menu Iniciar/aplicativos (quando suportado pelo sistema operacional).

## Estrutura esperada

O instalador presume que os artefatos da aplicação estejam disponíveis em um diretório `payload` localizado ao lado do executável gerado. Durante a instalação todos os arquivos são copiados recursivamente desse diretório para o destino escolhido.

```
qt-installer/
├── build/
├── payload/
│   └── ... arquivos da distribuição AnythingLLM ...
└── anything-llm-installer (binário gerado)
```

Ao final da instalação é criado (ou atualizado) o arquivo `installer-state.json` na pasta de configuração do usuário contendo o caminho e a versão instalada, permitindo que futuras execuções do instalador detectem o estado atual.

## Como compilar

1. Instale o Qt 6 (módulos *Widgets* e *Concurrent*) e o CMake 3.16 ou superior.
2. Gere um diretório de build e execute o CMake apontando para esta pasta:

   ```bash
   cmake -S extras/qt-installer -B extras/qt-installer/build -DAPP_VERSION="$(node -p "require('./package.json').version")"
   cmake --build extras/qt-installer/build --target anything-llm-installer
   ```

3. Copie os artefatos da aplicação para `extras/qt-installer/build/payload` antes de executar o instalador.

## Atalhos criados

* **Windows**: arquivos `.lnk` gerados via PowerShell na área de trabalho e no menu Iniciar.
* **Linux**: arquivos `.desktop` colocados na área de trabalho do usuário e em `~/.local/share/applications` (ou diretório equivalente retornado por `QStandardPaths`).
* **macOS**: criação de alias simbólico na área de trabalho.

## Personalização

As ações de instalação são encapsuladas em `InstallerLogic`, permitindo ajustes específicos, como validação de arquivos copiados, suporte a backups ou etapas adicionais pós-instalação.
