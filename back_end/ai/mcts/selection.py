import math


def select_child(node, c_puct, tolerance):
    '''
    TODO: What is `c_puct`?
    TODO: What is tolerance?
    TODO: What is the typical distribution of values of tolerance?
    Select a child node with the maximum PUCT score.
    If multiple children have scores within a small tolerance,
    choose the child with the lower visit count.
    If multiple children have scores within a small tolerance and the same visit count,
    choose the child with the higher prior probability.
    '''
    best_score = -float('inf')
    best_candidates = []
    for child in node.children.values():
        '''
        Value P of a node represents prior probability.
        Value N of a node represents visit count.
        TODO: Why is exploration bonus U calculated this way?
        TODO: What is `child.Q`?
        '''
        u = c_puct * child.P * math.sqrt(node.N) / (1 + child.N) # Calculate exploration bonus U
        score = child.Q + u # mean value Q + exploration bonus U
        if score > best_score + tolerance:
            best_score = score
            best_candidates = [child]
        elif abs(score - best_score) <= tolerance:
            best_candidates.append(child)
    if len(best_candidates) == 1:
        return best_candidates[0]
    best_candidates.sort(key = lambda c: (c.N, -c.P))
    return best_candidates[0]