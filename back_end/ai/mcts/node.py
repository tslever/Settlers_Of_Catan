class MCTS_Node:

    def __init__(self, game_state: dict, move = None, parent = None, move_type = None):
        '''
        Parameters:
            game_state: a dictionary representing the current game state
            move: the move that led to this node (for settlements, a vertex label; for roads, a tuple (edge, edge_key))
            parent: the parent MCTS node
            move_type: a string, either "settlement" or "road"
        '''
        self.game_state = game_state
        self.move = move
        self.parent = parent
        self.move_type = move_type
        self.children = {} # mapping from label of vertex for settlement, or key of edge for roads, to child node
        self.N = 0 # visit count
        self.W = 0.0 # total value
        self.Q = 0.0 # mean value (W / N)
        self.P = None # prior probability (set when expanding)
    
    def is_leaf(self):
        return len(self.children) == 0