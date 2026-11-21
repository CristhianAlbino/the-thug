#!/usr/bin/env python3
"""Render the optional dialogue bank into a readable creative review document."""

from banter_rules import DATA

DEST = __import__("pathlib").Path(__file__).resolve().parents[2] / "02_GAME_DESIGN/Narrative/DIALOGOS_AMBIENTAIS_V1.md"


def render():
    out = ["# Conversas ambientais — The Thug, texto v1",
           "",
           "**Estado:** primeira passada para revisão criativa. Fonte de dados: [`banter_ptbr.json`](../../Tools/NarrativeCore/banter_ptbr.json). Os 42 diálogos abaixo são opcionais; nenhum NPC surge só para entregar uma fala. Joe conhece o próprio passado, mas nenhuma conversa deduz a identidade civil de Batman. Conversas em todos os modos com risos habituais de Jack **não** registram J1–J3; somente os encontros cruéis específicos do Punitivo o fazem.",
           "",
           "**Disparo:** ato + área + cenas vistas + presença ativa de todos os interlocutores no mesmo nó. Perigo/perseguição ou cena principal em andamento suspendem conversas. Após 90 s desde a última conversa completa, escolher uma fala elegível ainda não ouvida; entregar ID apenas ao terminar. Se interromper, guardar ID/linha e tentar retomar com os mesmos atores; se dispersarem, cancelar sem conceder fatos. A ordem entre várias elegíveis é estável por ID até a futura direção espacial. Joe não pode usar conversa com ausente/capturado como fonte de informação.",
           ""]
    names = {"joe": "Joe", "harley": "Harley", "bartholomeu": "Bartholomeu", "jack": "Jack", "thomas": "Thomas"}
    for act in range(1, 6):
        out += [f"## Ato {act}", ""]
        for e in DATA["exchanges"]:
            if e["act"] != act:
                continue
            gate = ", ".join(e["nodes"])
            if e.get("after"):
                gate += "; após " + ", ".join(e["after"])
            if e.get("before"):
                gate += "; antes de " + ", ".join(e["before"])
            if e.get("requires_facts"):
                gate += "; após evidência " + ", ".join(e["requires_facts"])
            if e.get("modes"):
                gate += "; modos " + ", ".join(e["modes"])
            out += [f"### {e['id']} — {gate}", ""]
            for line in e["lines"]:
                out.append(f"**{names[line['speaker']]}:** “{line['text']}”  ")
            out.append("")
    out += ["## Origens de fatos condicionais", "",
            "| Fato | Só registrar após |", "| --- | --- |",
            "| `garage_camera_loop_observed` | Jack viu dois quadros repetidos no feed, não apenas entrou na sala. |",
            "| `camera_loop_observed_by_group` | Harley e Jack verificaram os quadros repetidos juntos. |",
            "| `doc11_read_by_jack` | Jack leu a placa/documento ou recebeu informação demonstrável no mesmo encontro. |",
            "| `camera_live_observed_by_group` | Joe, Harley e Jack verificaram timestamp/estado ao vivo juntos. |",
            "| `joe_delayed_harley_help` | Harley pediu ajuda e Joe realmente adiou; não registrar se o jogador ajudou logo. |",
            "| `harley_injured` | Ferimento físico confirmado, com Harley ainda ativa. |",
            "| `bartholomeu_cooperative` | Bartholomeu segue colaborando depois das ações reais de Joe. |",
            "| `bartholomeu_cooperated_n24` | Ele de fato ajudou em N24, sem abandono/traição naquele encontro. |",
            "| `helicopter_confirmed` | Rádio confirmou pouso e RunRules iniciou o prazo. |", "",
            "## Direção de continuidade", "",
            "- Thomas só participa antes de N03. Seus gestos com Bartholomeu e sua promessa de café dão algo concreto a lembrar; depois ele não fala como lembrança onisciente.",
            "- Joe menciona ter matado um casal Wayne no início de sua vida criminosa. Isso deve incomodar o jogador e os amigos; não absolver Joe nem revelar que Batman é o filho por dedução automática.",
            "- Harley mantém carinho por Bartholomeu e repulsa lúcida por Jack, mas reconhece quando ele dá informação verificável. Joe comum é frio em alguns momentos; no Punitivo demonstra reciprocidade.",
            "- Jack ri ao lembrar que sobreviveu ao Batman em qualquer modo. Sua reação inadequada a sofrimento e a trilha J1–J4 continuam exclusivas do Punitivo. A fala opcional não substitui prova testemunhada.",
            "- Textos não contêm palavrões fortes; ‘merda’, ‘bosta’ e ‘ferrou’ são o teto coloquial aprovado. Sons e música ainda serão produzidos depois da revisão.", ""]
    return "\n".join(out)


if __name__ == "__main__":
    DEST.write_text(render(), encoding="utf-8")
    print(f"{len(DATA['exchanges'])} conversas renderizadas em {DEST}")
