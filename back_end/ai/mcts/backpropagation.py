def backpropagate(node, value):
    '''
    Backpropagate the value estimated up the tree.
    '''
    while node is not None:
        node.N += 1 # visit count
        node.W += value # TODO: What is `node.W`?
        node.Q = node.W / node.N # TODO: What is `node.Q`?
        node = node.parent