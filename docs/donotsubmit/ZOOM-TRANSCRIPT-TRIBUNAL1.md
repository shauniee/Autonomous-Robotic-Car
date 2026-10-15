# Zoom briefing: tribunal 1

## 1. Executive Overview & Core Philosophy

### What is Tribunal 1?
Tribunal 1 is designed to stress-test how engineering teams respond when the requirements of an embedded system undergo unexpected changes mid-development. Up to this point, teams have designed submodules against baseline specifications; during Tribunal 1, those specifications will change.

### The Fundamental Question
> **"When the requirements of an embedded system change, what does that actually mean for your submodules?"**

The goal is **not** to immediately modify code. Instead, teams must conduct a thorough, structured, and deep engineering analysis of architectural, resource, timing, and dependency impacts across their subsystems.

### Core Grading Mindset
* **Sound Engineering Reasoning > Rubric Guesswork**: Do not attempt to reverse-engineer the rubric or chase arbitrary marks. If the discussion demonstrates sound embedded systems reasoning, top marks naturally follow.
* **Discussion Alone ≠ High Marks**: Mere talking or conversational volume does not score well. Depth, technical precision, and quantitative rigor determine grades.
* **Thought Process Over Answers**: The instructor evaluates the team's underlying rationale, risk analysis, and trade-off evaluation rather than just a simplistic "final fix."

## 2. Master Timeline & Critical Deadlines

All times are strictly enforced. Penalties apply for late submissions.

| Event / Deliverable | Date & Time | Description / Action Item |
| :--- | :--- | :--- |
| **Snapshot 1: Pre-Tribunal Submission** | **Mon, 19 Oct, Before 11:00 AM** | Upload `<GroupID>_pre-tribunal1.doc` *(or `.pdf`)* to Dropbox. Contains initial baseline design (Section 1). |
| **Week 8 Main Lecture Zoom** | **Mon, 19 Oct, 11:00 AM Sharp** | Mandatory attendance in the main Zoom lecture. Instructor presents the requirement changes (~15 min) and answers immediate Q&A. |
| **Team 90-Minute Breakout Discussion** | **Mon, 19 Oct, ~11:15 AM – 11:45 AM / 12:00 PM** | Teams break out into private Zoom/Teams meetings. Full session is recorded. Instructor hovers in main Zoom for drop-in support. |
| **Snapshot 2: Immediate Post-Tribunal Submission** | **Mon, 19 Oct, Before 12:10 PM Sharp** | Upload `<GroupID>_immediate-tribunal1.doc` *(or `.pdf`)* to Dropbox. Section 2 completed; Section 1 untouched. |
| **Video Recording Upload** | **Mon, 19 Oct (ASAP / By End of Day)** | Upload the full 90-minute video recording to Dropbox once processing finishes. |
| **Snapshot 3: Post-Tribunal Final Submission** | **Fri, 23 Oct, Before 23:59** | Upload `<GroupID>_post-tribunal1.doc` *(or `.pdf`)* to Dropbox. Section 3 completed; Sections 1 & 2 untouched. |

## 3. The Progressive Working Document (3-Snapshot System)

### One Continuous Document — Three Cumulative Snapshots
The deliverables are **not three separate documents**. They represent a single, evolving working document captured at three distinct development milestones:

```
[Snapshot 1: Pre-Tribunal]
└── Section 1: Pre-Tribunal Analysis (Completed before Mon 11:00 AM)

[Snapshot 2: Immediate Post-Tribunal]
├── Section 1: Pre-Tribunal Analysis (FROZEN / UNTOUCHED)
└── Section 2: Immediate Post-Tribunal Findings (Completed by Mon 12:10 PM)

[Snapshot 3: Final Post-Tribunal]
├── Section 1: Pre-Tribunal Analysis (FROZEN / UNTOUCHED)
├── Section 2: Immediate Post-Tribunal Findings (FROZEN / UNTOUCHED)
└── Section 3: Final Post-Tribunal Reflections & Corrections (Completed by Fri 23:59)
```

### Strict Snapshot Preservation Rules
1. **Never Overwrite Past Snapshots**: When moving from Snapshot 1 to 2, Section 1 must not be modified. When moving from Snapshot 2 to 3, Sections 1 and 2 must remain completely untouched.
2. **Document-Video Parity**: What appears in Snapshot 2 must tally exactly with what was discussed and typed during the recorded 90-minute video session. Retroactive tampering is considered an attempt at cheating.

### Policy on Mistakes & Self-Correction (Zero Penalty)
* If your team realizes during the post-tribunal analysis that an assumption, calculation, or conclusion made during the live 90-minute session was flawed, **do not alter Section 2**.
* Instead, document the error transparently in **Section 3 (Post-Tribunal)**:
  > *"During the live session, we concluded X. Upon deeper investigation, we found X was flawed because of Y, and the correct resolution is Z."*
* **Full credit is awarded**: The instructor explicitly stated that admitting, analyzing, and correcting mistakes in Section 3 carries **zero penalty** and positively demonstrates engineering growth and critical thinking.

## 4. Dropbox Final Deliverables Checklist

By **Friday, 23 October, 23:59**, the team's shared Dropbox folder must contain exactly **4 primary entries**:

| # | Item Name | File Format | File Naming Convention | Deadline |
| :-: | :--- | :--- | :--- | :--- |
| **1** | Snapshot 1: Pre-Tribunal | Word or PDF | `<GroupID>_pre-tribunal1.doc` *(or `.pdf`)* | Mon, 19 Oct, < 11:00 AM |
| **2** | Snapshot 2: Immediate Post-Tribunal | Word or PDF | `<GroupID>_immediate-tribunal1.doc` *(or `.pdf`)* | Mon, 19 Oct, < 12:10 PM |
| **3** | Snapshot 3: Post-Tribunal Final | Word or PDF | `<GroupID>_post-tribunal1.doc` *(or `.pdf`)* | Fri, 23 Oct, < 23:59 |
| **4** | Video Recording of Discussion | MP4 / WebM / Cloud | Standard video naming *(e.g. `<GroupID>_tribunal1_recording`)* | Mon, 19 Oct (ASAP) |
| *Opt* | Automated Discussion Transcript | `.txt` / `.vtt` / `.srt` | `<GroupID>_transcript.txt` | Recommended if using Zoom |

## 5. Week 8 Day-Of Protocol & Operational Workflow

```
11:00 AM ───────────────────────────────────────────────────────────┐
│ Phase 1: Main Lecture Zoom                                        │
│ • Everyone joins main room (no breakout rooms yet).               │
│ • Instructor briefs requirement changes (~15 min) + Live Q&A.     │
11:15 AM ───────────────────────────────────────────────────────────┘
│
▼
11:15 AM ───────────────────────────────────────────────────────────┐
│ Phase 2: 90-Minute Team Breakout Discussion                       │
│ • Move to private team meeting (MS Teams or Zoom).                │
│ • START RECORDING IMMEDIATELY (all members record redundancy).    │
│ • Webcams ON (faces visible) & Screen Share ON (AI queries).      │
│ • Deep technical scrutiny, cross-questioning, submodule impact.   │
│ • Instructor hovers in main Zoom for drop-in clarifications.      │
11:45 AM / 12:00 PM ────────────────────────────────────────────────┘
│
▼
11:45 AM – 12:10 PM ────────────────────────────────────────────────┐
│ Phase 3: Buffer & Immediate Upload                                │
│ • Finalize Section 2 notes and formatting.                        │
│ • Upload Snapshot 2 (<GroupID>_immediate-tribunal1.doc) by 12:10. │
12:10 PM Sharp ─────────────────────────────────────────────────────┘
```

### Instructor Drop-In Availability
During the entire 90-minute team breakout window, the instructor will remain online in the main lecture Zoom room. If the team needs clarification regarding requirement ambiguities or system constraints, members may pop back into the main Zoom room at any time.

## 6. Discussion Quality: Shallow Statements vs. Pointed Technical Questions

The instructor highlighted that high marks require moving from superficial acknowledgments to **pointed technical inquiries**:

| Technical Area | Shallow / Low-Scoring Approach | Pointed / High-Scoring Technical Analysis |
| :--- | :--- | :--- |
| **Task Execution** | *"The requirement changed, so I need to modify my task."* | *"If this task must now execute at 50 Hz instead of 10 Hz, what happens to CPU utilization and WCET (worst-case execution time)? Is the current RTOS priority still appropriate? Could higher-rate preemption starve time-critical background operations?"* |
| **Memory Allocation** | *"We need to store more data, so we'll just make the array bigger."* | *"Exactly how many additional bytes of RAM are required? Will this buffer be statically allocated in BSS or dynamically on the heap? Does the microcontroller have sufficient SRAM headroom, or does this risk stack-heap collision? Does an expanded ring buffer alter latency?"* |
| **Submodule Invariance** | *"My submodule has no changes. I don't need to do anything; I can relax."* | *"We examined the sensor polling change: while sample acquisition doubled, our internal rolling average filter consumes data at a fixed cadence that still meets Nyquist criteria. Existing buffer margins (64B) accommodate peak burst rates with 40% margin. Hence, API contracts and internal logic remain unchanged."* |
| **Hardware Constraints** | *"The hardware can probably support it."* | *"Does the peripheral bus (I2C/SPI) have enough bandwidth for the increased transfer rate? Will ADC sampling rate increases introduce quantization noise or DMA contention with serial communications?"* |
| **Inter-Module APIs** | *"I'll just change my return format."* | *"If Submodule A changes its payload data structure, which downstream submodules (B and C) are affected? Do we break existing API contracts? How will synchronization primitives (semaphores/mutexes) prevent race conditions under the new payload size?"* |

## 7. Embedded Systems Engineering Considerations Checklist (10 Dimensions)

When the requirements change, systematically evaluate your submodule across these ten core dimensions:

```
                          ┌──────────────────────────┐
                          │   REQUIREMENTS CHANGE    │
                          └─────────────┬────────────┘
                                        │
     ┌───────────────────┬──────────────┼──────────────┬───────────────────┐
     ▼                   ▼              ▼              ▼                   ▼
┌──────────────┐  ┌──────────────┐┌──────────────┐┌──────────────┐  ┌──────────────┐
│  1. Memory   │  │2. Processing ││  3. Timing   ││4. Concurrency│  │  5. Hardware │
│  RAM / ROM   │  │  CPU & WCET  ││  Deadlines   ││ ISR & Mutex  │  │ Peripherals  │
└──────────────┘  └──────────────┘└──────────────┘└──────────────┘  └──────────────┘
     ▲                   ▲              ▲              ▲                   ▲
     │                   │              │              │                   │
     └───────────────────┼──────────────┼──────────────┼───────────────────┘
     ▼                   ▼              ▼              ▼                   ▼
┌──────────────┐  ┌──────────────┐┌──────────────┐┌──────────────┐  ┌──────────────┐
│6. Performance│  │ 7. Code Size ││8. Interfaces ││9. Dependency │  │ 10. Trade-Off│
│  Throughput  │  │  Complexity  ││ API Contract ││ Ripple Effect│  │  Repercussion│
└──────────────┘  └──────────────┘└──────────────┘└──────────────┘  └──────────────┘
```

1. **Resources & Memory**:
   * Does RAM, ROM/Flash, stack, or heap consumption increase?
   * Are extra buffers, circular queues, state variables, or RTOS task control blocks required?
   * Is memory statically or dynamically allocated? What are the risks of fragmentation or overflow?
2. **Processing & CPU Utilization**:
   * How does the change alter computation cycles and CPU load?
   * Does it introduce algorithmic complexity that spikes worst-case execution time (WCET)?
3. **Timing & Deadlines**:
   * Do tasks execute more frequently or faster?
   * Have hard/soft real-time deadlines changed? Will the system still meet all deadlines under worst-case load?
4. **Performance & Scalability**:
   * Does throughput or latency degrade under peak burst conditions?
   * Can existing data structures handle increased data rates without packet drops?
5. **Code Size & Complexity**:
   * Does implementation bloat firmware footprint beyond Flash sector boundaries?
   * Does control logic become significantly more convoluted or fragile?
6. **Concurrency & Synchronization**:
   * Are new tasks or Interrupt Service Routines (ISRs) introduced?
   * Does contention arise for shared hardware or global data buffers?
   * Are mutexes, semaphores, queues, or critical sections appropriately configured to prevent race conditions or priority inversion?
7. **Interfaces & APIs**:
   * Are function signatures, parameter types, or return structs modified?
   * Does the API modification break backward compatibility with calling modules?
8. **Dependencies & Cascading Ripple Effects**:
   * If Submodule A changes its timing or output, how does it affect Submodules B, C, and D?
   * Does a change in one module force unplanned changes in dependent modules?
9. **Hardware Constraints**:
   * Can existing MCU clock speeds, GPIOs, timers, DMA channels, and communication buses (UART/SPI/I2C/CAN) handle the change?
   * To what extent does the hardware limit potential software fixes?
10. **Trade-offs & Repercussions**:
    * If you optimize one parameter (e.g., speed), what is sacrificed (e.g., memory, power consumption, code simplicity)?
    * What are the broader system repercussions of your architectural decisions?

## 8. Protocol for "Unaffected" Submodules

A frequent pitfall is team members prematurely declaring: *"My submodule has no changes; I can relax."*

### Key Rules:
1. **"No Change" Demands Engineering Justification**: Saying *"no change"* without detailed reasoning will result in mark deductions.
2. **The Justification Template**:
   * State the new requirement.
   * Quantify current capacity vs. new requirement demand (e.g., buffer size, execution margin, CPU headroom).
   * Verify interface stability (confirm APIs, parameters, and calling cadences remain unchanged).
   * Conclude with an engineering assertion of why no code modification is needed.
3. **Cross-Examination**: Teammates must cross-check each other's invariance claims:
   > *"If my module now sends sensor packets at 100 Hz instead of 10 Hz, and your module processes those packets, how can your module claim zero impact?"*

## 9. Rules of Engagement for AI Usage

Generative AI (e.g., ChatGPT) is explicitly authorized for Tribunal 1, provided the following ground rules are strictly observed:

* **Transparent Screen Sharing**: Any interaction with AI must be visible in the video recording. You may query AI concurrently on your individual screen, but when presenting to teammates, you **must share your screen** and show the prompts and responses.
* **No Penalty for AI Usage**: Marks are based on the depth of the engineering discussion, not whether AI was used.
* **Never Blindly Trust AI**: AI models hallucinate and provide simplistic or unfeasible embedded solutions. Team members must critically validate all AI suggestions.
* **Mutual Accountability & Peer Challenge**:
  * Team members must actively question and scrutinize ideas presented by peers.
  * If Member A presents a flawed or nonsensical AI suggestion, Member A may lose marks; however, if other teammates accept it uncritically without challenge, **the entire team is penalized**.
  * Challenging a teammate is **not** sabotage ("sabo-ing"); it is mandatory collaborative engineering rigor.
* **Individual Ownership**: Every member must speak and lead the discussion on their own submodule. Silence is interpreted as lack of preparation and ownership.

## 10. Video Recording & Academic Integrity Requirements

### Technical Recording Setup:
* **Platform**: Microsoft Teams or Zoom.
* **Redundant Recording**: It is strongly recommended that **all 4 to 5 members initiate local/cloud recordings concurrently**. A lost recording will result in heavy grade penalties.
* **Cameras ON**: Every member's face must be visible on camera during the discussion to verify presence and active engagement.
* **Video Quality**: Ultra-high definition is not required. Standard resolution is sufficient as long as audio is clear, faces are visible, and shared screens (AI prompts, diagrams, code) are legible.
* **Optional Transcript**: Submitting Zoom's automated transcript file to Dropbox alongside the video is highly welcomed.

### Academic Integrity & Discrepancy Checks:
* The instructor explicitly uses the video recording to **audit and validate** the contents of the Snapshot 2 document.
* Any discrepancy between what was typed/agreed during the video session and what was submitted by 12:10 PM will be treated as academic dishonesty.
* Do not tamper with Section 1 or Section 2 after submission. Discovered errors belong in Section 3.

## 11. Master Summary: Do's and Don'ts

| Category | DO | DON'T |
| :--- | :--- | :--- |
| **Preparation** | Submit Snapshot 1 before 11:00 AM on 19 Oct.<br>Ensure webcams and recording software are tested. | Do not miss the 11:00 AM main lecture Zoom.<br>Do not jump into breakout rooms early. |
| **Discussion** | Ask deep, pointed technical questions (timing, memory, concurrency).<br>Scrutinize teammates' submodule invariance claims.<br>Actively challenge flawed AI outputs. | Do not settle for superficial banter (*"Are you affected? OK, me neither"*).<br>Do not remain silent when teammates discuss your module.<br>Do not blindly trust AI suggestions. |
| **AI Usage** | Share your screen when reviewing AI prompts/outputs.<br>Use AI to explore edge cases, tradeoffs, and alternative designs. | Do not hide AI usage off-screen.<br>Do not copy-paste AI responses without peer validation. |
| **Snapshots & Edits** | Maintain a single evolving document across 3 snapshots.<br>Upload Snapshot 2 before 12:10 PM sharp.<br>Document mistakes openly in Section 3 (zero penalty). | Do not submit 3 disconnected files.<br>Do not retroactively edit Section 1 or Section 2.<br>Do not tamper with documents after the video session. |
| **Recording** | Have multiple members record concurrently.<br>Keep cameras ON and faces visible.<br>Upload video to Dropbox as soon as possible. | Do not lose the video recording (guaranteed penalty).<br>Do not turn off cameras during technical debates. |
