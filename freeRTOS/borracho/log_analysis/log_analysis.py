import pandas as pd
import matplotlib.pyplot as plt
from io import StringIO
import numpy as np

# Dados CSV completos gerado pelo log
dados_csv = """
TempoSeg;TempAtual;SaidaPWM;Curva
11;25.11;1023;1
12;25.10;1023;1
13;27.08;1023;1
14;31.87;1023;1
15;36.42;1023;1
16;40.75;1023;1
17;44.87;1023;1
18;48.79;1023;1
19;52.52;1023;1
20;56.06;1023;1
21;59.43;1023;1
22;62.64;1023;1
23;65.68;1023;1
24;68.58;968;1
25;71.34;864;1
26;73.73;762;1
27;75.32;673;1
28;76.20;600;1
29;76.50;544;1
30;76.42;499;1
31;76.09;463;1
32;74.88;461;1
33;74.10;449;1
34;73.30;438;1
35;72.49;432;1
36;71.69;425;1
37;70.91;426;1
38;70.21;428;1
39;69.55;434;1
40;68.97;439;1
41;68.47;445;1
42;68.03;452;1
43;67.69;457;1
45;67.43;462;1
46;67.24;466;1
47;67.11;469;1
48;67.03;471;1
49;67.00;472;1
50;67.00;472;1
51;67.03;471;1
52;67.08;469;1
53;67.15;466;1
54;67.23;463;1
55;67.31;459;1
56;67.39;455;1
57;67.46;451;1
58;67.49;447;1
59;67.49;445;1
60;67.45;444;1
61;67.39;444;1
62;67.30;445;1
63;67.20;447;1
64;67.09;449;1
65;66.96;453;1
66;66.82;458;1
67;66.69;464;1
68;66.55;470;1
69;66.42;477;1
70;66.29;484;1
71;66.17;492;1
72;66.05;500;1
73;65.96;508;1
74;65.87;517;1
75;65.74;527;1
76;65.68;535;1
77;65.65;543;1
78;65.62;550;1
79;65.61;557;1
81;65.60;564;1
82;65.60;571;1
83;65.61;578;1
84;65.63;584;1
85;65.65;590;1
86;65.67;596;1
87;65.70;602;1
88;65.76;606;1
89;65.86;609;1
90;66.00;610;1
91;66.17;610;1
92;66.36;607;1
93;66.56;604;1
94;66.76;599;1
95;66.96;593;1
96;67.15;587;1
97;67.33;580;1
98;67.63;568;1
100;67.76;561;1
101;67.87;554;1
102;67.96;547;1
103;68.04;540;1
104;68.09;253;2
105;68.13;291;2
106;66.66;378;2
107;65.51;463;2
108;64.84;538;2
109;64.68;599;2
110;64.94;647;2
111;65.58;680;2
112;66.47;702;2
113;67.49;714;2
114;68.51;721;2
115;70.49;689;2
117;71.42;688;2
118;72.27;689;2
119;73.05;682;2
120;73.75;674;2
121;74.38;665;2
122;74.93;656;2
123;75.40;646;2
124;75.79;636;2
125;76.10;627;2
126;76.36;618;2
127;76.55;610;2
128;76.67;603;2
129;76.74;597;2
130;76.77;592;2
131;76.75;589;2
132;76.71;587;2
134;76.53;590;2
135;76.40;591;2
136;76.25;594;2
137;76.10;597;2
138;75.96;601;2
139;75.81;606;2
140;75.68;611;2
141;75.55;617;2
142;75.42;624;2
143;75.31;630;2
144;75.21;637;2
145;75.13;644;2
146;75.06;650;2
147;75.00;657;2
148;74.95;664;2
149;74.89;671;2
151;74.88;677;2
152;74.87;683;2
153;74.89;687;2
154;74.94;692;2
155;75.04;694;2
156;75.18;694;2
157;75.35;692;2
158;75.55;689;2
159;75.76;684;2
160;75.97;678;2
161;76.18;671;2
162;76.38;664;2
163;76.55;656;2
164;76.71;648;2
166;76.98;635;2
167;77.08;627;2
168;77.16;619;2
169;77.22;611;2
170;77.27;604;2
171;77.30;597;2
172;77.32;590;2
173;77.32;583;2
174;77.32;577;2
175;77.31;571;2
176;77.29;565;2
177;77.26;559;2
178;77.24;554;2
179;77.21;548;2
180;77.18;543;2
182;77.13;539;2
"""

# --- Leitura e preparo dos dados ---
df = pd.read_csv(StringIO(dados_csv), sep=";")

df["TempoSeg"] = pd.to_numeric(df["TempoSeg"], errors="coerce")
df["TempAtual"] = pd.to_numeric(df["TempAtual"], errors="coerce")
df["SaidaPWM"] = pd.to_numeric(df["SaidaPWM"], errors="coerce")
df["Curva"] = pd.to_numeric(df["Curva"], errors="coerce")
df.dropna(inplace=True)

# --- Criação do gráfico ---
fig, ax1 = plt.subplots(figsize=(14, 6))

# === Temperatura ===
color = 'tab:blue'
ax1.set_xlabel("Tempo (s)")
ax1.set_ylabel("Temperatura (°C)", color=color)
ax1.plot(df["TempoSeg"], df["TempAtual"], color=color, label="Temperatura Atual", linewidth=2)
ax1.tick_params(axis='y', labelcolor=color)

# Escala Y de temperatura a cada 2°C
min_temp = np.floor(df["TempAtual"].min() / 2) * 2
max_temp = np.ceil(df["TempAtual"].max() / 2) * 2
ax1.set_yticks(np.arange(min_temp, max_temp + 2, 2))

# ... [mesmo código anterior até o plot] ...

# === Setpoints por curva ===
setpoints = {1: 67, 2: 76}
for curva_valor, setpoint in setpoints.items():
    curva_df = df[df["Curva"] == curva_valor]
    if not curva_df.empty:
        ax1.plot(
            curva_df["TempoSeg"], 
            [setpoint] * len(curva_df), 
            linestyle='--', 
            linewidth=1.5, 
            label=f"Setpoint Curva {curva_valor}"
        )

# === Linhas verticais de transição ===
if df["Curva"].nunique() > 1:
    transicoes = df[df["Curva"].diff() != 0]
    for _, row in transicoes.iterrows():
        ax1.axvline(x=row["TempoSeg"], color='red', linestyle='--', alpha=0.5)
        ax1.text(row["TempoSeg"], max_temp, f"Curva {int(row['Curva'])}", rotation=90, color='red', va='bottom')

# === PWM no segundo eixo ===
ax2 = ax1.twinx()
ax2.set_ylabel("Saída PWM", color='tab:green')
ax2.plot(df["TempoSeg"], df["SaidaPWM"], color='tab:green', label="PWM", linewidth=1.5, alpha=0.6)
ax2.tick_params(axis='y', labelcolor='tab:green')

# === Legenda ajustada ===
ax1.legend(loc="lower right")  # <- muda a posição da legenda aqui

# === Layout final ===
fig.tight_layout()
plt.title("Comportamento PID - Temperatura, PWM e Setpoints")
plt.grid(True)
plt.show()

# -------------------------
# === Cálculo de Métricas ===
# -------------------------
def calcular_metrica(curva_df, setpoint, curva_id):
    temp = curva_df["TempAtual"].values
    tempo = curva_df["TempoSeg"].values

    # Overshoot
    overshoot = max(temp) - setpoint

    # Tempo de subida (primeira vez que ultrapassa o setpoint)
    acima_setpoint = np.where(temp >= setpoint)[0]
    tempo_subida = tempo[acima_setpoint[0]] - tempo[0] if len(acima_setpoint) > 0 else None

    # Tempo de estabilização (dentro de ±1°C e fica estável nos últimos N segundos)
    margem = 1.0
    erro = np.abs(temp - setpoint)
    dentro_margem = erro <= margem
    tempo_estab = None
    for i in range(len(dentro_margem) - 10):
        if all(dentro_margem[i:i+10]):
            tempo_estab = tempo[i]
            break

    # Erro médio absoluto
    erro_medio = np.mean(np.abs(temp - setpoint))

    # Prints
    print(f"\n📊 Métricas da Curva {curva_id} (Setpoint = {setpoint}°C):")
    print(f"➡️ Overshoot: {overshoot:.2f} °C")
    print(f"⏱️ Tempo de subida: {tempo_subida:.2f} s" if tempo_subida is not None else "⏱️ Tempo de subida: não atingiu o setpoint")
    print(f"✅ Tempo de estabilização: {tempo_estab:.2f} s" if tempo_estab else "✅ Tempo de estabilização: não estabilizou")
    print(f"📉 Erro médio absoluto: {erro_medio:.2f} °C")

# Aplica a função para cada curva
for curva_id, setpoint in setpoints.items():
    curva_df = df[df["Curva"] == curva_id]
    if not curva_df.empty:
        calcular_metrica(curva_df, setpoint, curva_id)

