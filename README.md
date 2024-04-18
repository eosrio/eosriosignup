# eosriosignup
Automated Account creation smart contract

Code based on https://github.com/Dappub/signupeoseos

Improvements:
- Different Active and Owner keys
- Fixed ram amount (bytes) with on-demand price calculation
- Any excess funds are transferred as liquid native tokens

Contract is deployed on the mainnet under the account `eosriosignup`

## Usage Instructions

--- eosriosignup Project ---

- How to Build -
    - cd to 'build' directory
    - run the command 'cmake ..'
    - run the command 'make'

- After build -
    - The built smart contract is under the 'eosriosignup' directory in the 'build' directory
    - You can then do a 'set contract' action with 'cleos' and point in to the './build/eosriosignup' directory

- Additions to CMake should be done to the CMakeLists.txt in the './src' directory and not in the top level CMakeLists.txt