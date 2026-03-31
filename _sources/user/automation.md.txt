---
sort: 6
---

# Automation: press buttons automatically

The `--automation` argument allows to apply a set of actions (eg. button press)
when a condition is met (usually some text is displayed on the screen). It is
especially useful to automate app testing.

A JSON file is expected (either a JSON document or a path prefixed by `file:`).


## Rules

Rules are a list of rules. Each rule is a dictionary with the following valid
keys:

- `text`: expected text (string)
- `regexp`: regular expression matching the expected text (string)
- `x`: x-coordinate of the text (int)
- `y`: y-coordinate of the text (int)
- `conditions`: list of conditions, described below
- `actions`: list of actions, described below

Each key is optional.

### Actions

4 actions are available:

- `[ "button", num, pressed ]`: press (`pressed=true`) or release
  (`pressed=false`) a button (`num=1` for left, `num=2` for right)
- `[ "finger", x, y, touched ]`: touch (`touched=true`) or release
  (`touched=false`) the screen (`x` and `y` coordinates)
- `[ "setbool", varname, value ]`: set a variable whose name is `varname` to a
  boolean value (either `true` or `false`)
- `[ "exit" ]`: exit speculos

The actions of a rule are executed if and only if each rule option matches.
These actions are applied successively according to the ordering of the `actions`
list.

The actions of the first rule matched are applied. Further matching rules are
discarded (it allows to implement a *default* rule).

### Conditions

Conditions are a list of variables of tuple `(varname, value)` where `varname`
is a variable name and `value` a boolean. These variables are set by the
`setbool` action and by default an unset variable is equal to `false`.

If a non-empty `conditions` list is specified in the rule, each condition should
be met (as well as the other options) to allow the actions to be applied.


## Example

As an example, one can consider the following automation JSON file:

```json
{
    "version": 1,
    "rules": [
        {
            "text": "Application",
            "x": 35,
            "y": 3,
            "conditions": [
                [ "seen", false ]
            ],
            "actions": [
                [ "button", 2, true ],
                [ "button", 2, false ],
                [ "setbool", "seen", true ]
            ]
        },
        {
            "regexp": "\\d+",
            "actions": [
                [ "exit" ]
            ]
        },
        {
            "actions": [
                [ "setbool", "default_match", true ]
            ]
        }
    ]
}
```

The first rule matches only if:

- the text displayed at `(35, 3)` is `Application`
- this rule was never matched before (because the variable `seen` is set to
  `true` in the `actions` and expected to be `false` in the `conditions`)

If the first rule is matched, the right button (`2`) is pressed then released
(`true` then `false`) and the variable `seen` is set to `true`.

The second rules matches if the text is a number  (regular expression `\d+`)
displayed at any coordinates (no `x` nor `y` specified). The action `exit` makes
speculos exit without any confirmation.

The last rule is a default rule (there are no options). If no previous rule is
matched, the variable `default_match` is set to `true`.
