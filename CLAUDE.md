# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

TACHYS (Trading Architecture for Crypto & High-Yield Systems). A production-grade, low-latency trading system in modern C++ (C++20/23). Full spec in docs/TACHYS Technical Specification.pdf.

## Who I Am

Senior engineer/architect. 10 years professional C++ at Bloomberg (multithreaded distributed systems, finance). Now in management at JP Morgan (Python/React stack). Building this to stay sharp on C++ and eventually trade real capital.

## Communication

- Terse for routine updates. Show reasoning for trade-offs and architectural decisions.
- Never use emoji. Never use em dashes in prose.
- Don't explain concepts I already know (RAII, move semantics, templates, memory models). Do flag what's new in C++20/23 when relevant.
- Generate code fully (I'll review), but ask before starting. 99% of the time I'll say go.

## Workflow Orchestration

- Enter plan mode for any non-trivial task (3+ steps or architectural decisions). If something goes sideways, STOP and re-plan immediately.
- Use subagents liberally. One task per subagent. Offload research and exploration to keep the main context clean.
- Never mark a task complete without proving it works. Run tests, check logs, demonstrate correctness.
- For non-trivial changes, pause and ask "is there a more elegant way?" Skip this for simple, obvious fixes.

## Self-Improvement

- After ANY correction from me, update .claude/rules/lessons.md with a rule that prevents the same mistake. Review rules at session start.

## Bug Fixing

- Fix trivial issues autonomously (missing include, typo, obvious fix), tell me after.
- Stop and ask for anything architectural or that deviates from the spec.

## Git

- Feature branches per GitHub Issue. One PR per issue. Squash merge to main.
- Commit messages: short, why-focused.

## Sprint and Tracking

- sprint.md is the current sprint. archived_sprints/ for completed sprints.
- Sprint items are not done until: code committed, QA passed, training completed.
- GitHub Issues created upfront from PRDs with labels and milestones per phase.
- One standing "Complete Training" issue per sprint.

## Agent Team

- /trainer: C++ training partner. Manual invocation only. Explains first, then quizzes.
- /scrum: Sprint management and context recovery. Start and end of sessions.
- /qa: Code review and acceptance criteria checking.
- /researcher: External investigation (APIs, libraries, benchmarks).

Agent behavior defined in .claude/agents/. Enforcement rules in .claude/rules/.

## Core Principles

- Simplicity first. Make every change as simple as possible. Minimal code.
- No laziness. Find root causes. No temporary fixes. Senior developer standards.
- Minimal impact. Changes touch only what's necessary. Avoid introducing bugs.
