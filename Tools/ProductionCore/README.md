# ProductionCore — preparação de áudio e assets

`production_manifest.json` indexa os 314 cues atuais do diretor e das conversas, com IDs, texto/gesto original, ponteiro para a fonte e hash dos arquivos autorais. `requirements.json` reúne os lotes de produção complementar. O índice é material interno: alguns textos e nomes revelam conteúdo protegido. Não entregá-lo ao participante de teste cego.

```bash
python3 Tools/ProductionCore/build_manifest.py
python3 Tools/ProductionCore/build_manifest.py --check
```

Os campos `creative_review`, `planned` e `production_file: null` são intencionais. Gerar o índice não produz vozes, animações, modelos, música ou aprovação criativa. Os predicados do diretor continuam nas fontes de autoria; importar áudio nunca concede fatos nem confirma que uma fala foi percebida. Um cue só recebe ACK depois de apresentação real conforme DirectorCore.

As falas das novas interações estratégicas são direção futura: não acrescentar fala aleatória ou dica que revele intenção privada, rota protegida ou captura terminal. A recusa, o risco e o estado observado podem ser apresentados inicialmente com feedback visual e sonoro provisório. Novas falas exigem IDs/condições no diretor antes de integrar ao catálogo.

O [plano de produção](../../01_PROJECT/Roadmap/PLANO_PRODUCAO_PRE_UNREAL_V1.md) define entrega, avaliação e prioridade. Arquivos finais e avaliação humana ainda estão pendentes.
