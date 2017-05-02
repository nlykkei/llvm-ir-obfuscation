#!/usr/bin/python3

import sys
import re

DEBUG = False


def main():
    if len(sys.argv) < 6:
        print('Usage: {program} <program> <key> <prime> <prime>...'.format(program=sys.argv[0]))
        sys.exit(1)

    program = sys.argv[1]
    key = int(sys.argv[2], 16)
    primes = [int(sys.argv[i]) for i in range(3, len(sys.argv))]

    log('program: %s' % program)
    log('key: %x' % key)
    log('primes: %s' % primes)

    # EXTRACT
    wm_splits = []
    prefix = bytes(iter([0x66] * 15))

    input = sys.stdin.read().split()
    text_offset = int(input[2], 16)
    text_size = int(input[5], 16)

    with open(program, 'rb+') as f:
        f.seek(text_offset)
        for i in range(text_size - 15 + 1):
            bytes_read = f.read(15)
            if (bytes_read == bytes(iter([0x66] * 15))):
                log("Found byte sequence at position: %d" % int(text_offset + i))
                split_bytes = f.read(4)
                wm_split = int.from_bytes(split_bytes, byteorder='little', signed=False)
                wm_splits.append(wm_split)
                log("Read split: %d" % wm_split)
            f.seek(text_offset + i + 1)

    log('Extracted encryptions: %s' % repr(wm_splits))

    # DECRYPT
    for i in range(len(wm_splits)):
        wm_splits[i] = wm_splits[i] ^ key

    log('Decrypted splits: %s' % repr(wm_splits))

    #wm_splits.append(21)
    #wm_splits.append(4)
    #wm_splits.append(16)

    # COMBINE
    equations = []
    for i in range(len(wm_splits)):
        for j in range(len(primes) - 1):
            for k in range(j + 1, len(primes)):
                if wm_splits[i] >= primes[j] * primes[k]:
                    wm_splits[i] = wm_splits[i] - primes[j] * primes[k]
                else:
                    equations.append((wm_splits[i], primes[j], primes[k]))
                    break
            else:  # no break in loop
                continue
            break
            # ignore wm_splits[i] if value too large

    log('Equations: %s' % repr(equations))

    # GRAPH
    for p in primes:
        votes = [0] * p
        for equation in equations:
            if p == equation[1] or p == equation[2]:
                votes[equation[0] % p] += 1

        highest_vote = (None, None)
        next_highest_vote = (None, None)

        for i in range(len(votes)):
            if highest_vote[0] is None or highest_vote[1] < votes[i]:
                next_highest_vote = highest_vote
                highest_vote = (i, votes[i])

        if next_highest_vote[1] is not None and highest_vote[1] > 2 * next_highest_vote[1]:
            max = highest_vote[0]
            for equation in equations:
                if (p == equation[1] or p == equation[2]) and (equation[0] % p) != max:
                    equations.remove(equation)

    NG_set = set()
    NH_set = set()
    N_list = []
    for i in range(len(equations)):
        node = UGraph.Node(str(i), equations[i])
        NG_set.add(node)
        NH_set.add(node)
        N_list.append(node)

    G = UGraph('G', NG_set)
    H = UGraph('H', NH_set)

    #   Theory:
    #
    #   W = x mod p_1*p_2 (1)
    #   W = y mod q_1*q_2 (2)
    #
    #   Suppose p_1 = q_1 and x mod p_1 = y mod p_1, i.e., (1) and (2) are consistent:
    #
    #   (#1) If p_2 =! q_2 then x = y mod gcd(p_1*p_2, q_1*q_2) (=p_1)
    #
    #           => there exists a solution by CRT (OK)
    #
    #   (#2) If p_2 = q_2 and x mod p_2 = y mod p_2, i.e., (1) and (2) are consistent, then x = y mod p_1*p_2 (gcd(p_1, p_2) = 1)
    #
    #           => there exists a solution by CRT (OK)
    #
    #   (#3) If p_2 = q_2 and x mod p_2 != y mod p_2, i.e., (1) and (2) are inconsistent, then x != y mod p_1*p_2:
    #
    #           Proof: Suppose x = y mod p_1*p_2 then x = y mod p_2, a contradiction.
    #
    #           => there doesn't exist a solution by CRT (FAIL)
    #
    #   In case (#1) and (#2), the equations (1) and (2) could end up in U of consistent congruences, because there exists a solution by CRT.
    #
    #   In case (#3), both equations cannot end up in U, since they are neighbours in G. Thus, if one is selected for U, the other is deleted from H.
    #
    #   => Congruences (1) and (2) cannot both be consistent and inconsistent!
    #
    #   ... They can be either consistent or inconsistent or neither.
    #
    #   Invariant: equations in U were not adjacent in G.
    #
    #       => They are either pairwise consistent (only) or non-related
    #
    #   Given two equations in U, say (1) and (2):
    #
    #       - If they are consistent then a solution exist by CRT.
    #
    #       - If they are non-related then gcd(p_1*p_2, q_1*q_2) = 1, so a solution (clearly) exists by CRT.
    #
    #       => A unique solution exists modulo lcm().
    #
    #       => Since each equation can only contain p_i*p_j of original primes, the lcm() <= p_1*...*p_r.
    #
    #       => But, clearly, the original equations are consistent, so given these are chosen as part of U, lcm() = p_1*...*p_r.
    #
    #       => Thus, the solution must be W.

    for i in range(len(N_list) - 1):
        for j in range(i + 1, len(N_list)):
            cong1 = N_list[i].val
            cong2 = N_list[j].val

            # inconsistent
            if (((cong1[1] == cong2[1] or cong1[1] == cong2[2]) and
                     ((cong1[0] % cong1[1]) != (cong2[0] % cong1[1]))) or
                    ((cong1[2] == cong2[1] or cong1[2] == cong2[2]) and
                         ((cong1[0] % cong1[2]) != (cong2[0] % cong1[2])))):
                G.add_edge(UGraph.Edge(N_list[i], N_list[j]))

            # consistent
            if (((cong1[1] == cong2[1] or cong1[1] == cong2[2]) and
                     ((cong1[0] % cong1[1]) == (cong2[0] % cong1[1]))) or
                    ((cong1[2] == cong2[1] or cong1[2] == cong2[2]) and
                         ((cong1[0] % cong1[2]) == (cong2[0] % cong1[2])))):
                H.add_edge(UGraph.Edge(N_list[i], N_list[j]))

    equations = []  # set of consistent equations

    # H.print_graph()
    # G.print_graph()

    while not G.is_empty():
        v = H.max_degree_node()
        S = G.neighbours(v)
        G.remove_nodes(S)
        H.remove_nodes(S)
        equations.append(v.val)
        # H.print_graph()
        # G.print_graph()
        # print('Equations: ', equations, file=sys.stderr)

    # SOLUTION
    lcm_primes = None
    moduli = [equations[i][1] * equations[i][2] for i in range(len(equations))]

    if len(moduli) == 1:
        lcm_primes = moduli[0]
    else:
        lcm_primes = lcm_gen(moduli[0], moduli[1], *moduli[2:])

    log('Least-common-multiple: %d' % lcm_primes)

    for i in range(lcm_primes):
        success = True
        for j in range(len(equations)):
            res = (i - equations[j][0]) % (equations[j][1] * equations[j][2])
            if res != 0:
                success = False
                break  # try next solution
        if success:
            log('Solution: ', end='')
            print(i)
            sys.exit(0)

    print('Could not find a solution!')


class UGraph:
    def __init__(self, name, N=None, E=None):
        self.name = name
        if N is None:
            N = set()
        self.N = N

        if E is None:
            E = set()
        self.E = E

    def print_graph(self):
        print('Graph: ' + self.name, file=sys.stderr)
        print('N: ', file=sys.stderr, end='')
        for node in self.N:
            print(node, file=sys.stderr, end=' ')
        print(file=sys.stderr)
        print('E: ', file=sys.stderr, end=' ')
        for edge in self.E:
            print(edge, file=sys.stderr, end=' ')
        print(file=sys.stderr)

    def is_empty(self):
        return not self.N

    def get_node(self, name, val=None):
        for node in self.N:
            if node.name == name:
                if val is None or node.val == val:
                    return node
        return None

    def add_node(self, node):
        return self.N.add(node)

    def remove_node(self, node):
        try:
            self.N.remove(node)
        except KeyError as e:
            return False

        remove_edges = []

        for edge in self.E:
            if edge.start_node == node or edge.end_node == node:
                remove_edges.append(edge)

        for edge in remove_edges:
            try:
                self.E.remove(edge)
            except KeyError as e:
                return False

        return True

    def remove_nodes(self, nodes):
        for node in nodes:
            self.remove_node(node)

    def add_edge(self, edge):
        if edge.start_node in self.N and edge.end_node in self.N:
            return self.E.add(edge)
        return False

    def remove_edge(self, edge):
        try:
            self.E.remove(edge)
        except KeyError as e:
            return False
        return True

    def max_degree_node(self):
        nodes_dict = {node: 0 for node in self.N}
        for edge in self.E:
            nodes_dict[edge.start_node] += 1
            nodes_dict[edge.end_node] += 1
        return max(nodes_dict, key=lambda k: nodes_dict[k])

    def neighbours(self, node):
        neighbours = set([node])
        for edge in self.E:
            if edge.start_node == node:
                neighbours.add(edge.end_node)
            if edge.end_node == node:
                neighbours.add(edge.start_node)
        return neighbours

    class Node:
        def __init__(self, name, val):
            self.name = name
            self.val = val

        def __str__(self):
            return self.name + ' (' + repr(self.val) + ')'

        def __eq__(self, other):
            return self.name == other.name and self.val == other.val

        def __hash__(self):
            return hash(repr(self))

    class Edge:
        def __init__(self, start_node, end_node):
            self.start_node = start_node
            self.end_node = end_node

        def __str__(self):
            return self.start_node.name + ' -> ' + self.end_node.name

        def __eq__(self, other):
            return self.start_node == other.start_node and self.end_node == other.end_node

        def __hash__(self):
            return hash(repr(self))


def gcd(a, b):
    if b == 0:
        return a
    else:
        return gcd(b, a % b)


def lcm(a, b):
    return (a * b) / gcd(a, b)


def lcm_gen(a, b, *args):
    res = lcm(a, b)
    for mod in args:
        res = lcm(res, mod)
    return int(res)


def log(s, end='\n'):
    if DEBUG:
        print(s, end)


if __name__ == '__main__':
    main()
