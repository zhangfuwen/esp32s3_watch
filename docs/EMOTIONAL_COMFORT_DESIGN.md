# Emotional Comfort Features Design

## Vision
Transform the ESP32-S3 Watch from a timekeeping device into a **compassionate companion** that provides emotional support throughout the day.

---

## Core Principles

1. **Gentle, Not Intrusive** - Reminders should feel like a caring touch, not an alarm
2. **Private & Personal** - Emotional features should be discreet
3. **Always Available** - Comfort should be one button-press away
4. **Simple & Intuitive** - No complex menus during stressful moments
5. **Battery Conscious** - Comfort shouldn't drain the battery

---

## Feature Specifications

### 1. Breathing Companion 🌬️

**Purpose:** Guide users through calming breathing exercises

**Trigger:** Long-press POWER button (2 seconds)

**UI Design:**
```
    ╭─────────────╮
    │             │
    │    ╭───╮    │
    │   ╱     ╲   │  ← Expanding circle
    │  │  IN   │  │     (4 seconds)
    │   ╲     ╱   │
    │    ╰───╯    │
    │             │
    │   Breathe   │
    │    In...    │
    ╰─────────────╯
```

**Breathing Patterns:**
- **4-7-8 Relax:** In(4s) → Hold(7s) → Out(8s)
- **Box Breath:** In(4s) → Hold(4s) → Out(4s) → Hold(4s)
- **Coherent:** In(5s) → Out(5s) - heart rate variability optimization

**Haptic Feedback:**
- Gentle pulse at start of each phase
- Continuous soft vibration during exhale (calming)

**Exit:** Press any button or after 3 complete cycles

---

### 2. Mood Check-ins 💭

**Purpose:** Build emotional awareness through simple tracking

**Triggers:**
- Automatic: 9:00 AM, 2:00 PM, 9:00 PM (gentle double-vibration)
- Manual: Swipe down (if gesture supported) or menu option

**UI Design:**
```
    ╭─────────────╮
    │  How are    │
    │   you       │
    │  feeling?   │
    │             │
    │  😊 😐 😔   │  ← Button to cycle
    │             │
    │  [Good]     │  ← Current selection
    ╰─────────────╯
```

**Scale:** 1-5 (Very Bad → Very Good)

**Data Storage:**
- Local SPIFFS storage
- Date, time, mood score, optional tag (tired/stressed/calm/etc.)

**Weekly Review:**
- Simple bar chart showing mood distribution
- Accessible via long-press menu

---

### 3. Comfort Messages 💌

**Purpose:** Provide encouragement and positive reinforcement

**Display Triggers:**
- On wrist raise/wake
- After completing a breathing session
- Random gentle moments (1-2x per day)
- After mood check-in (personalized response)

**Message Categories:**

**Morning:**
- "Good morning! Today is a fresh start."
- "You have everything you need within you."
- "One step at a time. You've got this."

**Midday:**
- "Take a breath. You're doing great."
- "Pause. Breathe. Continue."
- "Progress, not perfection."

**Evening:**
- "You did enough today."
- "Rest is productive too."
- "Tomorrow is a new opportunity."

**Universal:**
- "You matter."
- "This feeling is temporary."
- "Be kind to yourself."
- "You are not alone."

**Implementation:**
- Stored in flash (user-customizable via config file)
- Rotated randomly, no repeats within 24 hours
- Optional: WiFi sync for new messages

---

### 4. Mindful Pause Reminders 🧘

**Purpose:** Gently nudge user to be present

**Timing:**
- 2-3 random times per day (not during active use)
- Avoid: Early morning, late night, during breathing exercises
- Smart: After 2+ hours of continuous wear

**UI Design:**
```
    ╭─────────────╮
    │             │
    │      🌿     │
    │             │
    │   Pause     │
    │   for a     │
    │   moment    │
    │             │
    │  [30s]      │  ← Optional countdown
    ╰─────────────╯
```

**Haptic:** Single gentle pulse (not repeated)

**Interaction:**
- Dismiss: Any button press
- Engage: Long-press for 30-second guided moment

---

### 5. Emergency Calm Button 🆘

**Purpose:** Immediate access to calming support during panic/stress

**Trigger:** Press BOTH buttons simultaneously (1 second)

**UI Design:**
```
    ╭─────────────╮
    │  ╭───────╮  │
    │ ╱  · · · ╲ │  ← Slow pulsing
    │ │    ·    │ │    calming animation
    │ ╲  · · · ╱ │
    │  ╰───────╯  │
    │             │
    │   You're    │
    │   safe      │
    │             │
    │  Breathe... │
    ╰─────────────╯
```

**Features:**
- Instant activation (no menu navigation)
- Soothing color transitions (if display supports)
- Slow breathing guide (6 breaths/minute)
- Optional: Very soft white noise via audio codec
- Auto-exit after 2 minutes or button press

**Priority:** Highest - interrupts any other screen

---

### 6. Connection Nudges 💕

**Purpose:** Remind user of social connections and support

**Triggers:**
- Weekly: "Call someone you love"
- After low mood entry: "Want to talk to someone?"
- Random: "You matter to someone"

**UI Design:**
```
    ╭─────────────╮
    │             │
    │     💕      │
    │             │
    │  Someone    │
    │  cares      │
    │  about you  │
    │             │
    ╰─────────────╯
```

**Tone:** Warm, not guilt-inducing

---

## Technical Implementation

### File Structure
```
main/
├── comfort/
│   ├── breathing.c      # Breathing companion
│   ├── breathing.h
│   ├── mood.c           # Mood tracking
│   ├── mood.h
│   ├── messages.c       # Comfort messages
│   ├── messages.h
│   ├── mindful.c        # Mindful reminders
│   ├── mindful.h
│   └── emergency.c      # Emergency calm
│       └── emergency.h
├── comfort_config.h     # Configuration structures
└── comfort_data.c       # SPIFFS data handling
```

### Hardware Usage
| Feature | Display | Haptic | Buttons | Audio |
|---------|---------|--------|---------|-------|
| Breathing | ✓ | ✓ | ✓ | ○ |
| Mood | ✓ | ✓ | ✓ | ○ |
| Messages | ✓ | ○ | ○ | ○ |
| Mindful | ✓ | ✓ | ✓ | ○ |
| Emergency | ✓ | ✓ | ✓ | ○ |
| Connection | ✓ | ○ | ○ | ○ |

✓ = Required, ○ = Optional future enhancement

### Power Management
- Breathing: ~5mA (2-3 min sessions)
- Mood check: ~2mA (30s interaction)
- Messages: Included in wake cost
- Mindful: ~2mA (30s, 2-3x/day)
- Emergency: ~5mA (2 min max)

**Estimated Daily Impact:** <10mAh (negligible)

### Data Privacy
- All mood data stored locally
- No cloud sync unless explicitly enabled
- User can delete all data via settings
- No identifiers, just timestamps + scores

---

## Success Metrics

**Qualitative:**
- User reports feeling calmer after using features
- Emergency calm used appropriately (not accidentally)
- Mood tracking feels helpful, not burdensome

**Quantitative:**
- Breathing sessions completed per week
- Mood check-in completion rate
- Emergency activation frequency
- Battery impact <5% per day

---

## Future Enhancements

**Phase 2 (Hardware Dependent):**
- HRV-based stress detection (if heart rate sensor added)
- Sleep quality correlation with mood
- Guided audio meditations via ES8311

**Phase 3 (Software):**
- WiFi message sync (loved ones can send encouragement)
- Mood pattern insights ("You're usually happier on weekends")
- Integration with calendar ("Big meeting today - you've got this!")

---

## Ethical Considerations

1. **Not a Medical Device** - Clear disclaimers needed
2. **Crisis Resources** - Emergency screen should show helpline info
3. **Data Sensitivity** - Mood data is highly personal
4. **Dependency Risk** - Features should empower, not create dependency

**Disclaimer Text:**
> "These features are for wellness support only and are not a substitute for professional mental health care. If you're in crisis, please contact a crisis helpline."

---

## Version History

- **v0.1.0** (2026-03-07): Initial design document
- **v0.2.0** (Planned): Add audio comfort features
- **v1.0.0** (Planned): Full feature set with WiFi sync
