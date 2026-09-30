"""Authored logical hospital layout. Run build_map.py to export JSON.

All distances and traversal seconds are blockout hypotheses, not playtest data.
IDs are stable integration keys; names and room dimensions may evolve.
"""

FLOORS = [
    ("G", "Garagem", -2),
    ("B1", "Subsolo técnico", -1),
    ("F1", "Emergência", 1),
    ("F2", "Administração", 2),
    ("F3", "Clínica", 3),
    ("F4", "Enfermarias e início", 4),
    ("R", "Telhado", 5),
    ("T", "Túneis", -3),
    ("A", "Anexo vizinho", 3),
    ("X", "Exterior", 0),
]

# (ID, displayed name, use in play, atmosphere tag)
NODES = [
    ("F4_START", "Quarto da ruptura", "inicio", "enfermaria"),
    ("F4_HALL_E", "Corredor leste", "circulacao", "luz_intermitente"),
    ("F4_HALL_W", "Corredor oeste", "circulacao", "piso_danificado"),
    ("F4_NURSE", "Posto de enfermagem", "pista_codigo_a", "papel_e_vidro"),
    ("F4_WARD_E", "Enfermaria leste", "esconderijos", "leitos"),
    ("F4_BATH", "Banhos e sanitários", "desvio", "azulejo_umido"),
    ("F4_STORAGE", "Depósito de lençóis", "recurso", "estantes"),
    ("F4_WARD_W", "Enfermaria oeste", "esconderijos", "leitos"),
    ("F4_OBSERVATION", "Sala da janela do Bat-Sinal", "marco_narrativo", "cidade"),
    ("F4_SERVICE", "Prumada de serviço", "atalho", "tubulacao"),
    ("F4_ST_E", "Escada leste 4", "circulacao", "escada"),
    ("F4_ST_W", "Escada oeste 4", "circulacao", "escada"),
    ("F4_LIFT", "Elevador 4", "atalho_condicional", "elevador"),
    ("F3_ST_E", "Escada leste 3", "circulacao", "escada"),
    ("F3_ST_W", "Escada oeste 3", "circulacao", "escada"),
    ("F3_CENTRAL", "Corredor clínico central", "circulacao", "pe_direito_alto"),
    ("F3_SURGERY", "Centro cirúrgico", "risco", "aco_e_luz_fria"),
    ("F3_ICU", "UTI", "esconderijos", "monitores"),
    ("F3_RECOVERY", "Recuperação", "circulacao", "cortinas"),
    ("F3_EXAMS", "Exames", "pista_janela", "radiologia"),
    ("F3_ISOLATION", "Isolamento antigo", "abrigo_raro", "porta_pesada"),
    ("F3_UTILITY", "Utilidades", "atalho", "tubulacao"),
    ("F3_PIPE", "Passagem entre tubulações", "atalho_estreito", "tubulacao"),
    ("F3_HALL_W", "Corredor clínico oeste", "circulacao", "sombra"),
    ("F3_WINDOW", "Sala de procedimento da janela", "rota_janela", "anexo_visivel"),
    ("F3_LIFT", "Elevador 3", "atalho_condicional", "elevador"),
    ("F2_ST_E", "Escada leste 2", "circulacao", "escada"),
    ("F2_ST_W", "Escada oeste 2", "circulacao", "escada"),
    ("F2_CENTRAL", "Galeria administrativa", "circulacao", "concreto"),
    ("F2_ADMIN", "Administração", "circulacao", "escritorios"),
    ("F2_ARCHIVE", "Arquivo físico", "pista_codigo_b", "papel"),
    ("F2_SECURITY", "Segurança", "pista_cofre", "vidro_escuro"),
    ("F2_CAMERAS", "Sala de câmeras", "informacao_interferencia", "telas"),
    ("F2_SERVER", "Sala de servidores", "pilhas", "racks"),
    ("F2_SAFE", "Sala do cofre", "chave_garagem", "metal"),
    ("F2_MEETING", "Sala de reuniões", "desvio", "mesa_longa"),
    ("F2_SUPPLY", "Almoxarifado", "ferramentas_janela", "estantes"),
    ("F2_SERVICE", "Galeria técnica", "atalho", "tubulacao"),
    ("F2_LIFT", "Elevador 2", "atalho_condicional", "elevador"),
    ("F1_ST_E", "Escada leste 1", "circulacao", "escada"),
    ("F1_ST_W", "Escada oeste 1", "circulacao", "escada"),
    ("F1_LOBBY", "Átrio da entrada", "exposicao", "vidro_e_chuva"),
    ("F1_EMERGENCY", "Emergência", "circulacao", "portas_duplas"),
    ("F1_TRIAGE", "Triagem", "desvio", "biombos"),
    ("F1_PHARMACY", "Farmácia", "tratamento", "vitrine"),
    ("F1_RADIO", "Sala de rádio", "rota_telhado", "interferencia"),
    ("F1_SECURITY", "Guarita interna", "circulacao", "cameras"),
    ("F1_LOADING", "Doca de insumos", "atalho", "portao"),
    ("F1_TREATMENT", "Pronto atendimento", "tratamento", "macas"),
    ("F1_SERVICE", "Acesso à prumada", "atalho", "tubulacao"),
    ("F1_LIFT", "Elevador 1", "atalho_condicional", "elevador"),
    ("B1_ST_E", "Escada leste subsolo", "circulacao", "escada"),
    ("B1_ST_W", "Escada oeste subsolo", "circulacao", "escada"),
    ("B1_CENTRAL", "Espinha de manutenção", "circulacao", "concreto_umido"),
    ("B1_GENERATOR", "Geradores", "energia_pista_codigo_c", "maquinas"),
    ("B1_LAUNDRY", "Lavanderia", "desvio", "vapor"),
    ("B1_MORGUE", "Necrotério", "risco", "gavetas"),
    ("B1_PIPE", "Galeria de tubulações", "atalho", "tubulacao"),
    ("B1_PUMP", "Casa de bombas", "orientacao_tuneis", "agua"),
    ("B1_STORAGE", "Depósito técnico", "recurso", "caixas"),
    ("B1_TUNNEL", "Entrada não mapeada", "rota_tunel", "parede_falsa"),
    ("B1_LIFT", "Elevador subsolo", "atalho_condicional", "elevador"),
    ("B1_VALVE", "Galeria de válvulas", "atalho", "tubulacao"),
    ("G_ST_E", "Escada leste garagem", "circulacao", "escada"),
    ("G_ST_W", "Escada oeste garagem", "circulacao", "escada"),
    ("G_FLOOR", "Pátio central da garagem", "exposicao", "pilares"),
    ("G_VEHICLES", "Fileiras de veículos", "esconderijos", "farol"),
    ("G_CONTROL", "Controle do portão", "rota_garagem", "quadro_eletrico"),
    ("G_GATE", "Portão de saída", "rota_garagem", "luz_exterior"),
    ("G_MAINT", "Oficina da garagem", "atalho", "ferramentas"),
    ("G_TUNNEL_LINK", "Acesso de serviço dos túneis", "atalho", "tubulacao"),
    ("T_FORK", "Bifurcação subterrânea", "orientacao", "agua_rasa"),
    ("T_DEAD_A", "Ramal sem saída norte", "beco_sem_saida", "paredes_umidas"),
    ("T_PUMP", "Conduto das bombas", "circulacao", "tubulacao"),
    ("T_VALVE", "Câmara de válvulas", "circulacao", "tubulacao"),
    ("T_LOOP", "Anel de manutenção", "orientacao", "marcas"),
    ("T_DEAD_B", "Ramal sem saída leste", "beco_sem_saida", "paredes_umidas"),
    ("T_EXIT", "Galeria de descarga", "rota_tunel", "vento_exterior"),
    ("R_ST_E", "Escada leste telhado", "circulacao", "escada"),
    ("R_ST_W", "Escada oeste telhado", "circulacao", "escada"),
    ("R_HVAC", "Máquinas de climatização", "cobertura", "dutos"),
    ("R_PAD", "Área de pouso improvisada", "rota_telhado", "cidade"),
    ("R_LEDGE", "Parapeito", "risco", "vento"),
    ("A_BRIDGE", "Passarela de serviço do anexo", "rota_janela", "altura"),
    ("X_GARAGE", "Rua pela garagem", "saida", "cidade"),
    ("X_TUNNEL", "Saída pluvial", "saida", "cidade"),
    ("X_WINDOW", "Edifício vizinho", "saida", "cidade"),
    ("X_ROOF", "Extração no telhado", "saida", "cidade"),
]

# Edges are undirected by default. seconds = traversal estimate without chase.
EDGES = []


def link(a, b, kind="corridor", seconds=12, requires=(), forbids=(),
         one_way=False, hunter=True, exposure="medium"):
    EDGES.append(dict(a=a, b=b, kind=kind, seconds=seconds,
                      requires=list(requires), forbids=list(forbids),
                      one_way=one_way, hunter_access=hunter,
                      exposure=exposure))


# Wide public routes form loops; connecting rooms do not merely add length.
for a, b in [
    ("F4_START", "F4_HALL_E"), ("F4_START", "F4_OBSERVATION"),
    ("F4_HALL_E", "F4_NURSE"), ("F4_HALL_E", "F4_WARD_E"),
    ("F4_HALL_E", "F4_ST_E"), ("F4_HALL_E", "F4_LIFT"),
    ("F4_NURSE", "F4_HALL_W"), ("F4_WARD_E", "F4_BATH"),
    ("F4_BATH", "F4_HALL_W"), ("F4_HALL_W", "F4_WARD_W"),
    ("F4_HALL_W", "F4_ST_W"), ("F4_WARD_W", "F4_STORAGE"),
    ("F4_STORAGE", "F4_SERVICE"), ("F4_SERVICE", "F4_HALL_W"),
    ("F4_OBSERVATION", "F4_HALL_W"),
    ("F3_ST_E", "F3_CENTRAL"), ("F3_CENTRAL", "F3_SURGERY"),
    ("F3_CENTRAL", "F3_ICU"), ("F3_CENTRAL", "F3_RECOVERY"),
    ("F3_CENTRAL", "F3_LIFT"), ("F3_SURGERY", "F3_UTILITY"),
    ("F3_UTILITY", "F3_PIPE"), ("F3_UTILITY", "F3_HALL_W"),
    ("F3_ICU", "F3_RECOVERY"), ("F3_RECOVERY", "F3_EXAMS"),
    ("F3_EXAMS", "F3_WINDOW"), ("F3_EXAMS", "F3_HALL_W"),
    ("F3_HALL_W", "F3_ST_W"), ("F3_HALL_W", "F3_ISOLATION"),
    ("F3_WINDOW", "F3_HALL_W"),
    ("F2_ST_E", "F2_CENTRAL"), ("F2_CENTRAL", "F2_ADMIN"),
    ("F2_CENTRAL", "F2_SECURITY"), ("F2_CENTRAL", "F2_SERVER"),
    ("F2_CENTRAL", "F2_LIFT"), ("F2_ADMIN", "F2_ARCHIVE"),
    ("F2_ADMIN", "F2_SAFE"), ("F2_ARCHIVE", "F2_MEETING"),
    ("F2_MEETING", "F2_ST_W"), ("F2_SECURITY", "F2_CAMERAS"),
    ("F2_CAMERAS", "F2_SERVICE"), ("F2_SERVICE", "F2_ST_W"),
    ("F2_SERVICE", "F2_SUPPLY"), ("F2_SUPPLY", "F2_SERVER"),
    ("F1_ST_E", "F1_EMERGENCY"), ("F1_EMERGENCY", "F1_LOBBY"),
    ("F1_EMERGENCY", "F1_TRIAGE"), ("F1_EMERGENCY", "F1_RADIO"),
    ("F1_TRIAGE", "F1_PHARMACY"), ("F1_PHARMACY", "F1_TREATMENT"),
    ("F1_TREATMENT", "F1_LOBBY"), ("F1_LOBBY", "F1_SECURITY"),
    ("F1_SECURITY", "F1_ST_W"), ("F1_SECURITY", "F1_RADIO"),
    ("F1_ST_W", "F1_LOADING"), ("F1_LOADING", "F1_SERVICE"),
    ("F1_SERVICE", "F1_RADIO"), ("F1_EMERGENCY", "F1_LIFT"),
    ("B1_ST_E", "B1_CENTRAL"), ("B1_CENTRAL", "B1_GENERATOR"),
    ("B1_CENTRAL", "B1_LAUNDRY"), ("B1_CENTRAL", "B1_PIPE"),
    ("B1_CENTRAL", "B1_LIFT"), ("B1_LAUNDRY", "B1_MORGUE"),
    ("B1_MORGUE", "B1_ST_W"), ("B1_PIPE", "B1_VALVE"),
    ("B1_PIPE", "B1_PUMP"), ("B1_PUMP", "B1_TUNNEL"),
    ("B1_VALVE", "B1_ST_W"), ("B1_VALVE", "B1_STORAGE"),
    ("B1_STORAGE", "B1_GENERATOR"),
    ("G_ST_E", "G_FLOOR"), ("G_FLOOR", "G_VEHICLES"),
    ("G_FLOOR", "G_GATE"), ("G_FLOOR", "G_CONTROL"),
    ("G_VEHICLES", "G_ST_W"), ("G_VEHICLES", "G_MAINT"),
    ("G_MAINT", "G_TUNNEL_LINK"), ("G_MAINT", "G_CONTROL"),
    ("T_FORK", "T_DEAD_A"), ("T_FORK", "T_PUMP"),
    ("T_FORK", "T_VALVE"), ("T_PUMP", "T_LOOP"),
    ("T_VALVE", "T_LOOP"), ("T_LOOP", "T_DEAD_B"),
    ("T_LOOP", "T_EXIT"), ("R_ST_E", "R_HVAC"),
    ("R_ST_W", "R_HVAC"), ("R_HVAC", "R_PAD"),
    ("R_PAD", "R_LEDGE"),
]:
    link(a, b)

# The service lattice connects floors but never removes the Hunter's ability
# to follow by another route or inspect an endpoint.
for a, b in [
    ("F4_ST_E", "F3_ST_E"), ("F3_ST_E", "F2_ST_E"),
    ("F2_ST_E", "F1_ST_E"), ("F1_ST_E", "B1_ST_E"),
    ("B1_ST_E", "G_ST_E"), ("F4_ST_W", "F3_ST_W"),
    ("F3_ST_W", "F2_ST_W"), ("F2_ST_W", "F1_ST_W"),
    ("F1_ST_W", "B1_ST_W"), ("B1_ST_W", "G_ST_W"),
    ("F4_ST_E", "R_ST_E"), ("F4_ST_W", "R_ST_W"),
]:
    link(a, b, kind="stairs", seconds=20, exposure="high")

for a, b in [
    ("F4_LIFT", "F3_LIFT"), ("F3_LIFT", "F2_LIFT"),
    ("F2_LIFT", "F1_LIFT"), ("F1_LIFT", "B1_LIFT"),
]:
    link(a, b, kind="lift", seconds=12,
         requires=("power_restored",), exposure="high")

for a, b in [
    ("F4_SERVICE", "F3_PIPE"), ("F3_PIPE", "F2_SERVICE"),
    ("F2_SERVICE", "F1_SERVICE"), ("F1_SERVICE", "B1_PIPE"),
    ("B1_VALVE", "G_TUNNEL_LINK"),
]:
    link(a, b, kind="pipe_crawl", seconds=24, hunter=False,
         exposure="low")

link("B1_TUNNEL", "T_FORK", kind="tunnel", seconds=18,
     requires=("tunnel_found",), exposure="low")
link("G_TUNNEL_LINK", "T_VALVE", kind="tunnel", seconds=20,
     requires=("tunnel_found",), exposure="low")
link("G_GATE", "X_GARAGE", kind="exit", seconds=15,
     requires=("gate_released",), forbids=("garage_failed",), exposure="high")
link("T_EXIT", "X_TUNNEL", kind="exit", seconds=18,
     requires=("tunnel_found",), forbids=("tunnel_failed",), exposure="medium")
link("F3_WINDOW", "A_BRIDGE", kind="window", seconds=35,
     requires=("window_unblocked",), forbids=("window_failed",),
     one_way=True, hunter=False, exposure="high")
link("A_BRIDGE", "X_WINDOW", kind="exit", seconds=25,
     forbids=("window_failed",), one_way=True, exposure="high")
link("R_PAD", "X_ROOF", kind="exit", seconds=10,
     requires=("helicopter_confirmed",), forbids=("roof_failed",),
     exposure="high")

# Actions have explicit prerequisites; no authored clue sits beyond the exit
# it unlocks. Codes are six digits, but the digits themselves are undecided.
ACTIONS = [
    ("clue_code_a", "F4_NURSE", (), "code_a", None),
    ("clue_code_b", "F2_ARCHIVE", (), "code_b", None),
    ("clue_code_c", "B1_GENERATOR", (), "code_c", None),
    ("restore_power", "B1_GENERATOR", (), "power_restored", None),
    ("learn_key", "F1_SECURITY", (), "key_known", None),
    ("locate_safe", "F2_SECURITY", ("key_known",), "safe_known", None),
    ("open_safe", "F2_SAFE", ("safe_known", "code_a", "code_b", "code_c"),
     "safe_opened", "SafeOpened"),
    ("take_garage_key", "F2_SAFE", ("safe_opened",),
     "garage_key", "GarageKeyTaken"),
    ("release_gate", "G_CONTROL", ("garage_key", "power_restored"),
     "gate_released", "GarageGateReleased"),
    ("escape_garage", "X_GARAGE", ("gate_released",),
     "garage_escaped", "GarageExitReached"),
    ("find_radio", "F1_RADIO", (), "radio_found", "RadioFound"),
    ("find_batteries", "F2_SERVER", (), "batteries_found", None),
    ("clear_interference", "F2_CAMERAS", (), "interference_clear", None),
    ("install_batteries", "F1_RADIO", ("radio_found", "batteries_found"),
     "batteries_installed", "BatteriesInstalled"),
    ("make_radio_contact", "F1_RADIO",
     ("batteries_installed", "interference_clear"),
     "radio_contact", "RadioContactEstablished"),
    ("confirm_helicopter", "F1_RADIO", ("radio_contact",),
     "helicopter_confirmed", "HelicopterConfirmed"),
    ("escape_roof", "X_ROOF", ("helicopter_confirmed",),
     "roof_escaped", "RoofReached"),
    ("discover_tunnel", "B1_TUNNEL", (), "tunnel_found", "TunnelFound"),
    ("escape_tunnel", "X_TUNNEL", ("tunnel_found",),
     "tunnel_escaped", "TunnelExitReached"),
    ("discover_window", "F3_WINDOW", (), "window_found", "WindowFound"),
    ("find_window_tools", "F2_SUPPLY", (), "window_tools", "WindowToolsFound"),
    ("unblock_window", "F3_WINDOW", ("window_found", "window_tools"),
     "window_unblocked", "WindowUnblocked"),
    ("escape_window", "X_WINDOW", ("window_unblocked",),
     "window_escaped", "WindowCrossed"),
    ("discover_isolation", "F3_UTILITY", (), "isolation_known", None),
]

# This is an information gate, not a locked mandatory escape path.
for edge in EDGES:
    if {edge["a"], edge["b"]} == {"F3_HALL_W", "F3_ISOLATION"}:
        edge["requires"] = ["isolation_known"]

SIGHTLINES = [
    ("F4_OBSERVATION", "R_LEDGE", "silhouette_above"),
    ("F3_CENTRAL", "F2_CENTRAL", "atrium_partial"),
    ("F2_CAMERAS", "F1_LOBBY", "camera_if_powered"),
    ("G_FLOOR", "G_GATE", "headlights_and_rain"),
]

EXITS = {
    "garage": {"node": "X_GARAGE", "flag": "garage_escaped",
               "failure": "garage_failed"},
    "roof": {"node": "X_ROOF", "flag": "roof_escaped",
             "failure": "roof_failed"},
    "tunnel": {"node": "X_TUNNEL", "flag": "tunnel_escaped",
               "failure": "tunnel_failed"},
    "window": {"node": "X_WINDOW", "flag": "window_escaped",
               "failure": "window_failed"},
}


def build():
    return {
        "schema": 1,
        "source": "GDD_MASTER sections 22-31, 48, 53-56, 65 and 81",
        "start": "F4_START",
        "floors": [dict(id=i, name=n, order=o) for i, n, o in FLOORS],
        "nodes": [dict(id=i, floor=i.split("_")[0], name=n,
                       role=r, mood=m) for i, n, r, m in NODES],
        "edges": EDGES,
        "actions": [dict(id=i, at=n, requires=list(req), grants=flag,
                         run_event=event) for i, n, req, flag, event in ACTIONS],
        "sightlines": [dict(a=a, b=b, cue=cue)
                       for a, b, cue in SIGHTLINES],
        "exits": EXITS,
    }
