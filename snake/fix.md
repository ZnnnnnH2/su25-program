Short answer: not correct yet. Your code records trap cells but never actually uses them to avoid stepping on traps, and it also mis-scores trap value (-20 vs the rules’ -10). Details + fixes below.

# What the rules/API expect

* Traps are items with `type == -2`; stepping on one is allowed but **deducts points** (not lethal, shield doesn’t help). The rules specify **−10 points**. &#x20;

# What your code does now

* You parse items and assign `value = -20` when `type == -2` (comment also says “扣20分”), which **doesn’t match** the spec (should be −10).&#x20;
* You mark traps into `GridMask.trap_rows` in `build_masks`, but:

  * `bfs_grid(...)` never adds any extra cost for entering a trap cell.
  * The fallback “survival” chooser (`last_choice`) checks `blocked/danger/snake` but **doesn’t check `is_trap`**, so it can freely move onto a trap even when avoidable.&#x20;
* You already skip traps as targets (good), but paths are still allowed to go through traps at zero planning cost.&#x20;

# Minimal, safe fixes

Below are focused edits that (1) fix the trap value, (2) discourage stepping on traps in the planner, and (3) avoid traps in the safety fallback, unless there’s no alternative.

1. Fix the parsed trap value to match the rules (−10):

```cpp
// read_state(...)
case -2: // 陷阱
    s.items[i].value = -10; // <- rules say -10
    break;
```

&#x20;

2. Add a trap penalty into the path search:

* Add a constant:

```cpp
static constexpr int TRAP_STEP_COST = 30; // discourage entering traps
```

* In `bfs_grid(...)`, when expanding `(ny,nx)`:

```cpp
int extra = 0;
if (M.is_trap(ny, nx)) {
    extra += TRAP_STEP_COST; // soft-avoid traps
}
int new_total_cost = new_dist + new_snake_cost * SNAKE_COST_WEIGHT + extra;
```

This makes the planner prefer non-trap routes but still allows traps if truly necessary. (You can tune `TRAP_STEP_COST`—30 works well relative to your `SNAKE_COST_WEIGHT=10`.)

3. Avoid traps in the “safe move” fallback:
   In `last_choice()`’s “SAFE\_MOVE\_ANALYSIS”, add one more guard:

```cpp
if (M.is_trap(ny, nx)) { 
    log_ss << ":TRAP|"; 
    continue; 
}
```

Keep the later “DESPERATE\_MOVE\_ANALYSIS” as-is so the snake can still step on a trap when every other option is worse than dying.

4. Optional: final pre-move guard
   Right before returning the “NORMAL\_MOVE”, you already check danger. Consider also avoiding a trap on the very next cell if there’s an equally safe alternative:

```cpp
if (M.is_trap(cy, cx)) {
    log_ss << "TRAP_NEXT_STEP:|";
    int choice = last_choice();
    return {choice};
}
```

(This keeps you from walking onto a trap as the immediate step when your fallback can pick a clean tile.)

# Why these choices

* Rules compliance: −10 points for traps, not lethal, shield irrelevant; the fixes align with that. &#x20;
* Soft constraints: trapping shouldn’t be “blocked” (you might need to cross one to survive), so we model it as **cost**, not a hard wall.
* Fallback safety: when you’re cornered, it’s better to lose 10 points than die—so we only hard-avoid traps when there are other safe tiles.

# Quick checklist you can test

* Spawn a trap adjacent to the head with an open alternative path:

  * Expected: planner prefers the non-trap route.
* Box the snake so only a trap cell is free:

  * Expected: it moves onto the trap (loses points) rather than die.
* Verify parsed item value: print/log a trap item’s `.value` and confirm it’s −10.

If you want, I can produce a unified diff against your `/mnt/data/my-snake.cpp` with these exact changes applied.
