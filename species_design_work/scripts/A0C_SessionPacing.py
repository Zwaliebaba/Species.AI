"""A0 challenger check: what the world-time targets of gdd/00 mean in real time.

Inputs: none. Every value below is copied from the tuning table of
Design/gdd/00_vision_pillars_and_core_loops.md (section 4), or is an assumption
named ASSUMED. Output: plain-text tables on stdout.

Run: python3 -I species_design_work/scripts/A0C_SessionPacing.py
"""

import math

# --- Values from gdd/00 section 4 -------------------------------------------
BATTLES_PER_WEEK = (2, 10)          # pil.world.battles_per_week, per settlement, world time
AWAY_YIELD = (0.50, 0.65, 0.80)     # pil.guard.away_yield_share, low / mid / high
PLATEAU_DAYS = 14                   # pil.settlement.plateau_days, world time
LEADER_RATIO = 3.0                  # pil.guard.leader_strength_ratio
LATE_SHARE = 0.50                   # pil.guard.late_joiner_strength_share
LATE_HOURS_ACTIVE = 6               # pil.guard.late_joiner_hours, of active play
CHECK_WEEK = 4                      # pil.guard.check_world_week

# --- Session patterns (minutes of presence per day) -------------------------
PATTERNS = [
    ("check-in only", 10),
    ("light", 30),
    ("typical", 60),
    ("heavy", 120),
    ("very heavy", 240),
]

# ASSUMED: a respite of 8 hours of world time (B6 sets the real value).
RESPITE_HOURS_ASSUMED = 8


def DailyGrowth(_presentMinutes, _awayYield):
    """Growth over one day of world time, in units of 'one hour of a present, competent steward'."""
    presentHours = _presentMinutes / 60.0
    return presentHours + (24.0 - presentHours) * _awayYield


def Section(_title):
    print()
    print(_title)
    print("-" * len(_title))


def main():
    Section("1. Dedicated world running 24 h a day: share of world time a steward is present")
    for name, minutes in PATTERNS:
        print(f"  {name:14s} {minutes:4d} min/day  present {100.0 * minutes / 1440.0:5.1f} % of world time")

    Section("2. PIL-023: daily growth relative to the heavy (120 min/day) steward")
    heavy = 120
    for awayYield in AWAY_YIELD:
        row = []
        for name, minutes in PATTERNS:
            row.append(f"{name}={DailyGrowth(minutes, awayYield) / DailyGrowth(heavy, awayYield):.3f}")
        never = DailyGrowth(0, awayYield) / DailyGrowth(heavy, awayYield)
        print(f"  away yield {awayYield:.2f}: " + ", ".join(row) + f", never present={never:.3f}")
    for awayYield in AWAY_YIELD:
        gain = DailyGrowth(heavy, awayYield) / DailyGrowth(10, awayYield) - 1.0
        print(f"  away yield {awayYield:.2f}: 120 min/day out-grows 10 min/day by {100.0 * gain:.1f} % for 12x the play")

    Section("3. PIL-028: largest strength ratio that presence alone can create")
    print(f"  always present / never present = 1 / {AWAY_YIELD[0]:.2f} = {1.0 / AWAY_YIELD[0]:.2f}"
          f"  (guard limit {LEADER_RATIO:.1f}); between real patterns (240 vs 10 min/day):"
          f" {DailyGrowth(240, AWAY_YIELD[0]) / DailyGrowth(10, AWAY_YIELD[0]):.2f}")

    Section("4. Battles a steward fights in person, dedicated 24 h world (timing uniform; attackers who pick away times give 0)")
    for name, minutes in PATTERNS:
        share = minutes / 1440.0
        low, high = (b * share for b in BATTLES_PER_WEEK)
        print(f"  {name:14s} {low:4.2f} to {high:4.2f} battles in person per week, of {BATTLES_PER_WEEK[0]} to {BATTLES_PER_WEEK[1]}")

    Section("5. Integrated world (stage 1): world time passes only while the host plays")
    for name, minutes in PATTERNS[1:]:
        hoursPerDay = minutes / 60.0
        plateauRealDays = PLATEAU_DAYS * 24.0 / hoursPerDay
        respiteRealDays = RESPITE_HOURS_ASSUMED / hoursPerDay
        print(f"  host plays {minutes:3d} min/day: the 14-day plateau takes {plateauRealDays:6.0f} real days;"
              f" week {CHECK_WEEK} of world time is {CHECK_WEEK * 168.0 / hoursPerDay:5.0f} real days;"
              f" an 8 h respite lasts {respiteRealDays:4.1f} real days")
    low, high = BATTLES_PER_WEEK
    print(f"  solo steward: one battle every {168.0 / high:.1f} to {168.0 / low:.1f} hours of play"
          f" ({168.0 / high / 2.0:.1f} to {168.0 / low / 0.5:.0f} sessions of 30 to 120 minutes);"
          f" the plateau after {PLATEAU_DAYS * 24} hours of play")

    Section("6. PIL-029: growth speed-up a week-4 late joiner needs to reach 50 % of the median")
    plateauHours = PLATEAU_DAYS * 24.0
    tau = plateauHours / 3.0          # saturating curve that reaches 95 % at the plateau
    linearHalf = LATE_SHARE * plateauHours
    saturatingHalf = tau * math.log(1.0 / (1.0 - LATE_SHARE))
    for label, worldHours in [("6 h played in one sitting", 6.0),
                              ("6 h at 120 min/day (3 days)", 72.0),
                              ("6 h at 60 min/day (6 days)", 144.0)]:
        print(f"  {label:30s} world time {worldHours:5.0f} h: linear growth needs {linearHalf / worldHours:5.1f}x,"
              f" saturating growth {saturatingHalf / worldHours:5.2f}x")

    Section("7. Ghost settlements (ASSUMED churn: half the stewards of a world stop playing each month, and every place is refilled)")
    places = 16
    for months in (1, 3, 6):
        ghosts = places * 0.5 * months
        print(f"  after {months} month(s): {places} active settlements and {ghosts:.0f} deputy-run ghosts,"
              f" each growing at {AWAY_YIELD[0] * 100:.0f} to {AWAY_YIELD[2] * 100:.0f} % of an attentive steward")


if __name__ == "__main__":
    main()
