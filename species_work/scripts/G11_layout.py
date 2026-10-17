"""G11: distances between tutorial map roles (reference layout). Usage: python3 -I G11_layout.py"""
import math
t1 = {'home_node': (652.31, 858.62), 'second_node': (1157.24, 845.36), 'third_node': (1570.37, 686.83),
      'home_gather': (699.05, 906.38), 'second_gather': (1163.51, 912.70), 'third_gather': (1599.06, 724.60),
      'centre_zone': (1208.74, 1400.50), 'far_zone': (1844.11, 749.73), 'crate_point': (1466.79, 1083.31),
      'enemy_home_node': (629.54, 2014.80)}
t2 = {'home_node': (2744.23, 1627.55), 'bot2_home': (545.70, 2528.04), 'bot1_home': (1228.58, 325.25),
      'first_console': (2520.41, 1646.10), 'first_dish': (2469.69, 1702.33), 'target_dish': (2007.97, 1819.35),
      'centre_zone': (1594.59, 1595.19), 'attack_node': (1262.11, 1643.24), 'bot_gate_node': (1309.43, 1645.08),
      'crate_point': (1806.21, 1758.78), 'transport_goal': (1938.30, 2824.31), 'transport_place': (1846.76, 1927.39),
      'final_console': (1892.22, 2507.58), 'final_dish': (1809.98, 2558.16), 'final_target': (1669.89, 1877.06)}
def d(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])
pairs1 = [('home_node', 'home_gather'), ('second_node', 'second_gather'), ('third_node', 'third_gather'),
          ('home_node', 'second_node'), ('second_node', 'third_node'), ('second_node', 'centre_zone'),
          ('third_node', 'far_zone'), ('crate_point', 'centre_zone'), ('home_node', 'enemy_home_node')]
pairs2 = [('first_console', 'home_node'), ('first_console', 'first_dish'), ('first_dish', 'target_dish'),
          ('target_dish', 'centre_zone'), ('bot_gate_node', 'attack_node'), ('attack_node', 'bot2_home'),
          ('attack_node', 'home_node'), ('attack_node', 'centre_zone'), ('transport_place', 'transport_goal'),
          ('final_console', 'final_dish'), ('final_dish', 'final_target'), ('crate_point', 'transport_place'),
          ('home_node', 'centre_zone')]
for n, pairs, t in (('first', pairs1, t1), ('second', pairs2, t2)):
    for a, b in pairs:
        print(n, a, b, round(d(t[a], t[b]), 1))
