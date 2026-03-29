# TACHYS Development Workflow Design

## Overview

Design for an AI-augmented solo development workflow to build TACHYS (Trading Architecture for Crypto & High-Yield Systems), a production-grade low-latency trading system in modern C++ (C++20/23). The developer is a senior engineer/architect with 10 years of C++ at Bloomberg, now in management. The project serves dual purposes: building a real trading system and maintaining C++ sharpness.

## Layer Architecture

```
CLAUDE.md
  Personal preferences, conventions, project context.
  Never contains agent/skill logic.

Agents (.claude/agents/)
  Roles that orchestrate skills. Can evolve to compose multiple skills.
  /trainer  /scrum  /qa  /researcher

Skills
  Atomic enforced capabilities. 1:1 mapping with agents for now.
  Can split into finer-grained skills as the project evolves.

File System
  sprint.md, archived_sprints/, docs/prd/, docs/trainer/, docs/research/
  GitHub Issues as source of truth for history.
```

## Agent Definitions

### /trainer -- Technical Training Partner

- Role: Peer-level C++ mentor calibrated to Bloomberg background
- Trigger: Manual only. Never auto-invoked.
- Behavior:
  - Explains the concept/pattern first, in context of what was built
  - Then quizzes to test understanding
  - Interactive Q&A mode on demand
  - Focuses on modern C++20/23 delta and subtle gotchas
  - Generates small exercises and snippets tied to code that was generated
  - Assumes deep knowledge of RAII, move semantics, templates, memory models
  - Tracks progress in docs/trainer/progress.md
  - Logs sessions in docs/trainer/sessions/YYYY-MM-DD-topic.md
- Sprint gate: Sprint items cannot be marked done until associated training is completed

### /scrum -- Sprint Master

- Role: Context recovery and sprint management
- Trigger: Start and end of every session, or on demand
- Behavior:
  - Reads sprint.md, summarizes current state, recommends what to tackle
  - Updates sprint.md at end of session with what was done, what's next, blockers
  - Archives completed sprints to archived_sprints/sprint-YYYY-MM-DD.md
  - Creates GitHub Issues from PRD items, closes them when done
  - Flags overdue items, suggests scope cuts
  - Enforces sprint gate: checks code committed + QA passed + training completed before closing items

### /qa -- Quality Assurance

- Role: Code reviewer and acceptance criteria verifier
- Trigger: After code is written, before marking a sprint item done
- Behavior:
  - Reviews code against the spec's acceptance criteria for the current phase
  - Checks hot-path violations: heap allocations, virtual dispatch, system calls, mutex usage
  - Runs/suggests static analysis (AddressSanitizer, clang-tidy)
  - Verifies benchmarks meet the spec's targets
  - Checks test coverage

### /researcher -- Technical Investigator

- Role: Explores external information
- Trigger: On demand
- Behavior:
  - Investigates exchange APIs (Binance, Bybit, Polymarket)
  - Compares C++ library options
  - Finds benchmark references and best practices
  - Saves findings to docs/research/<topic>.md

## Skills

1:1 mapping with agents. Each skill encodes the enforced behavior for its corresponding agent.

| Skill | Capabilities |
|-------|-------------|
| trainer | Explain concepts, generate quizzes, track progress, log sessions |
| scrum | Sprint file management, session bookkeeping, issue sync, gate enforcement |
| qa | Acceptance criteria checking, hot-path validation, code review |
| researcher | API investigation, library comparison, output formatting |

Skills can split into finer-grained units as the project evolves. Agents would then compose multiple skills.

## File Structure

```
tachys/
  CLAUDE.md                            # Personal preferences, code standards only
  sprint.md                            # Current sprint, single source of truth
  archived_sprints/
    sprint-YYYY-MM-DD.md               # Completed sprints, immutable
  docs/
    TACHYS Technical Specification.pdf  # Full spec (exists)
    prd/
      phase-0-environment.md           # PRD per phase, generated from spec
      phase-1-feed-handler.md
      ...
    trainer/
      progress.md                      # What's been taught, quiz results, gaps
      sessions/
        YYYY-MM-DD-topic.md            # Individual training session logs
    research/
      <topic>.md                       # Researcher output
  .claude/
    settings.json                      # Exists
    agents/
      trainer.md
      scrum.md
      qa.md
      researcher.md
    rules/
      lessons.md                       # Self-improvement, grows over time
      code-style.md                    # Created in Phase 0
  .github/
    (issues synced from PRDs)
  src/                                 # C++ project, set up in Phase 0
```

## Sprint File Format (sprint.md)

```markdown
# Sprint N -- Phase X: Title
**Started:** YYYY-MM-DD
**Target:** YYYY-MM-DD (2 weeks)
**Status:** In Progress

## Goals
- [ ] Goal 1
- [ ] Goal 2

## Items

### Item Name
- **Issue:** #N
- **PR:** #N (or "not started")
- **Status:** Pending | In Progress | Done
- **Training:** Not started | In Progress | Complete
- **Notes:**

### Complete Training
- **Issue:** #N (standing issue per sprint)
- **Status:** Pending
- **Notes:** Linked to all items above

## Session Log

### YYYY-MM-DD (X hrs)
- What was done
- Next: what's planned
- Blockers: any blockers

## Retrospective
(filled on sprint close)
```

## Sprint Lifecycle

1. **Session start**: invoke /scrum. It reads sprint.md, shows where you left off, suggests what to tackle.
2. **During session**: build code. Invoke /trainer when ready to learn. Invoke /qa when ready to review.
3. **Session end**: invoke /scrum. It appends session log, updates statuses.
4. **Sprint close**: /scrum checks all items (code + QA + training). If training incomplete, sprint stays open. Completed sprint moves to archived_sprints/.
5. **New sprint**: /scrum pulls next items from relevant PRD, creates GitHub Issues, writes new sprint.md.

## Sprint Gate Rule

An item is done only when all three are true:
- Code committed (PR merged)
- QA acceptance criteria passed
- Training for that item marked complete in docs/trainer/progress.md

## GitHub Issues Strategy

- All issues created upfront from PRDs for all 8 phases
- Organized with labels (phase-0, phase-1, ...) and milestones (Phase 0, Phase 1, ...)
- One standing "Complete Training" issue per sprint
- Each issue maps to a component/deliverable from the spec
- PRDs generated from the full technical specification

## Git Workflow

- Feature branches per GitHub Issue
- One PR per issue
- Squash merge to main
- Commit messages: short, why-focused

## CLAUDE.md Scope

CLAUDE.md contains only:
- Project description
- Developer profile (for agent calibration)
- Communication preferences
- High-level workflow orchestration rules
- Core principles (simplicity, no laziness, minimal impact)
- Sprint and tracking conventions (brief)
- Agent team roster (names and one-liners only)

It never contains: agent behavior details, skill definitions, code style rules, or anything that belongs in .claude/agents/ or .claude/rules/.

## Decisions Log

- Code style: deferred to Phase 0 (clang-format setup)
- Skill granularity: 1:1 with agents for now, can split later
- Issues: all created upfront with labels/milestones
- Autonomy: generate code fully, ask before starting, trainer provides exercises after
- Bug fixing: fix trivial issues autonomously, stop and ask for architectural decisions
