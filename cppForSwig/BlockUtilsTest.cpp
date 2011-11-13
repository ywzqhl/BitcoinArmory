#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "UniversalTimer.h"
#include "BinaryData.h"
#include "BtcUtils.h"
#include "BlockUtils.h"
#include "EncryptionUtils.h"


using namespace std;

void copyFile(string src, string dst)
{
   fstream fin(src.c_str(), ios::in | ios::binary);
   fstream fout(dst.c_str(), ios::out | ios::binary);
   if(fin == NULL || fout == NULL) { cout <<"error"; return; }
   // read from the first file then write to the second file
   char c;
   while(!fin.eof()) { fin.get(c); fout.put(c); }
}


////////////////////////////////////////////////////////////////////////////////
void TestReadAndOrganizeChain(string blkfile);
void TestFindNonStdTx(string blkfile);
void TestScanForWalletTx(string blkfile);
void TestReorgBlockchain(string blkfile);
void TestCrypto(void);
////////////////////////////////////////////////////////////////////////////////

void printTestHeader(string TestName)
{
   cout << endl;
   for(int i=0; i<80; i++) cout << "*";
   cout << "Execute test: " << TestName << endl;
   for(int i=0; i<80; i++) cout << "*";
}

int main(void)
{

   //string blkfile("home/alan/.bitcoin/blk0001.dat");
   string blkfile("C:/Documents and Settings/VBox/Application Data/Bitcoin/blk0001.dat");

   //printTestHeader("Read-and-Organize-Blockchain");
   //TestReadAndOrganizeChain(blkfile);

   //printTestHeader("Find-Non-Standard-Tx");
   //TestFindNonStdTx(blkfile);

   //printTestHeader("Wallet-Relevant-Tx-Scan");
   //TestScanForWalletTx(blkfile);

   //printTestHeader("Blockchain-Reorg-Unit-Test");
   //TestReorgBlockchain(blkfile);

   printTestHeader("Crypto-KDF-and-AES-methods");
   TestCrypto();


   /////////////////////////////////////////////////////////////////////////////
   // ***** Print out all timings to stdout and a csv file *****
   //       Any method, anywhere, that called UniversalTimer
   //       will end up having it's named timers printed out
   //       This file can be loaded into a spreadsheet,
   //       but it's not the prettiest thing...
   UniversalTimer::instance().print();
   UniversalTimer::instance().printCSV("timings.csv");
   cout << endl << endl;
   char pause[256];
   cout << "enter anything to exit" << endl;
   cin >> pause;
}







void TestReadAndOrganizeChain(string blkfile)
{
   BlockDataManager_FullRAM & bdm = BlockDataManager_FullRAM::GetInstance(); 
   /////////////////////////////////////////////////////////////////////////////
   cout << "Reading data from blockchain..." << endl;
   TIMER_START("BDM_Load_and_Scan_BlkChain");
   bdm.readBlkFile_FromScratch(blkfile, false);  // don't organize, just index
   TIMER_STOP("BDM_Load_and_Scan_BlkChain");
   cout << endl << endl;

   /////////////////////////////////////////////////////////////////////////////
   cout << endl << "Organizing blockchain: " ;
   TIMER_START("BDM_Organize_Chain");
   bool isGenOnMainChain = bdm.organizeChain();
   TIMER_STOP("BDM_Organize_Chain");
   cout << (isGenOnMainChain ? "No Reorg!" : "Reorg Detected!") << endl;
   cout << endl << endl;

   /////////////////////////////////////////////////////////////////////////////
   // TESTNET has some 0.125-difficulty blocks which violates the assumption
   // that it never goes below 1.  So, need to comment this out for testnet
#ifndef TEST_NETWORK
   cout << "Verify integrity of blockchain file (merkleroots leading zeros on headers)" << endl;
   TIMER_START("Verify blk0001.dat integrity");
   bool isVerified = bdm.verifyBlkFileIntegrity();
   TIMER_STOP("Verify blk0001.dat integrity");
   cout << "Done!   Your blkfile " << (isVerified ? "is good!" : " HAS ERRORS") << endl;
   cout << endl << endl;
#endif
}



void TestFindNonStdTx(string blkfile)
{
   BlockDataManager_FullRAM & bdm = BlockDataManager_FullRAM::GetInstance(); 
   bdm.readBlkFile_FromScratch(blkfile, false);  // don't organize, just index
   // This is mostly just for debugging...
   bdm.findAllNonStdTx();
   // At one point I had code to print out nonstd txinfo... not sure
   // what happened to it...
}



void TestScanForWalletTx(string blkfile)
{
   BlockDataManager_FullRAM & bdm = BlockDataManager_FullRAM::GetInstance(); 
   bdm.readBlkFile_FromScratch(blkfile, false);  // don't organize, just index
   /////////////////////////////////////////////////////////////////////////////
   BinaryData myAddress;
   BtcWallet wlt;
   
#ifndef TEST_NETWORK
   // Main-network addresses
   myAddress.createFromHex("604875c897a079f4db88e5d71145be2093cae194"); wlt.addAddress(myAddress);
   myAddress.createFromHex("8996182392d6f05e732410de4fc3fa273bac7ee6"); wlt.addAddress(myAddress);
   myAddress.createFromHex("b5e2331304bc6c541ffe81a66ab664159979125b"); wlt.addAddress(myAddress);
   myAddress.createFromHex("ebbfaaeedd97bc30df0d6887fd62021d768f5cb8"); wlt.addAddress(myAddress);
#else
   // Test-network addresses
   myAddress.createFromHex("abda0c878dd7b4197daa9622d96704a606d2cd14"); wlt.addAddress(myAddress);
   myAddress.createFromHex("11b366edfc0a8b66feebae5c2e25a7b6a5d1cf31"); wlt.addAddress(myAddress);
   myAddress.createFromHex("baa72d8650baec634cdc439c1b84a982b2e596b2"); wlt.addAddress(myAddress);
   myAddress.createFromHex("fc0ef58380e6d4bcb9599c5369ce82d0bc01a5c4"); wlt.addAddress(myAddress);
#endif

   myAddress.createFromHex("0e0aec36fe2545fb31a41164fb6954adcd96b342"); wlt.addAddress(myAddress);

   TIMER_WRAP(bdm.scanBlockchainForTx_FromScratch(wlt));
   
   cout << "Checking balance of all addresses: " << wlt.getNumAddr() << " addrs" << endl;
   for(uint32_t i=0; i<wlt.getNumAddr(); i++)
   {
      BinaryData addr20 = wlt.getAddrByIndex(i).getAddrStr20();
      cout << "  Addr: " << wlt.getAddrByIndex(i).getBalance() << ","
                         << wlt.getAddrByHash160(addr20).getBalance() << endl;
      vector<LedgerEntry> const & ledger = wlt.getAddrByIndex(i).getTxLedger();
      for(uint32_t j=0; j<ledger.size(); j++)
      {  
         cout << "    Tx: " 
           << ledger[j].getAddrStr20().getSliceCopy(0,4).toHexStr() << "  "
           << ledger[j].getValue()/(float)(CONVERTBTC) << " (" 
           << ledger[j].getBlockNum()
           << ")  TxHash: " << ledger[j].getTxHash().getSliceCopy(0,4).toHexStr() << endl;
      }

   }
   cout << endl << endl;



   cout << "Printing SORTED allAddr ledger..." << endl;
   wlt.sortLedger();
   vector<LedgerEntry> const & ledgerAll = wlt.getTxLedger();
   for(uint32_t j=0; j<ledgerAll.size(); j++)
   {  
      cout << "    Tx: " 
           << ledgerAll[j].getAddrStr20().toHexStr() << "  "
           << ledgerAll[j].getValue()/1e8 << " (" 
           << ledgerAll[j].getBlockNum()
           << ")  TxHash: " << ledgerAll[j].getTxHash().getSliceCopy(0,4).toHexStr() << endl;
           
   }


   /////////////////////////////////////////////////////////////////////////////
   cout << "Test txout aggregation, with different prioritization schemes" << endl;
   BtcWallet myWallet;

#ifndef TEST_NETWORK
   // TODO:  I somehow borked my list of test addresses.  Make sure I have some
   //        test addresses in here for each network that usually has lots of 
   //        unspent TxOuts
   
   // Main-network addresses
   myAddress.createFromHex("0e0aec36fe2545fb31a41164fb6954adcd96b342"); myWallet.addAddress(myAddress);
#else
   // Testnet addresses
   //myAddress.createFromHex("d184cea7e82c775d08edd288344bcd663c3f99a2"); myWallet.addAddress(myAddress);
   //myAddress.createFromHex("205fa00890e6898b987de6ff8c0912805416cf90"); myWallet.addAddress(myAddress);
   //myAddress.createFromHex("fc0ef58380e6d4bcb9599c5369ce82d0bc01a5c4"); myWallet.addAddress(myAddress);
#endif

   cout << "Rescanning the blockchain for new addresses." << endl;
   bdm.scanBlockchainForTx_FromScratch(myWallet);

   vector<UnspentTxOut> sortedUTOs = bdm.getUnspentTxOutsForWallet(myWallet, 1);

   int i=1;
   cout << "   Sorting Method: " << i << endl;
   cout << "   Value\t#Conf\tTxHash\tTxIdx" << endl;
   for(int j=0; j<sortedUTOs.size(); j++)
   {
      cout << "   "
           << sortedUTOs[j].getValue()/1e8 << "\t"
           << sortedUTOs[j].getNumConfirm() << "\t"
           << sortedUTOs[j].getTxHash().toHexStr() << "\t"
           << sortedUTOs[j].getTxOutIndex() << endl;
   }
   cout << endl;
}



void TestReorgBlockchain(string blkfile)
{
   BlockDataManager_FullRAM & bdm = BlockDataManager_FullRAM::GetInstance(); 
   /////////////////////////////////////////////////////////////////////////////
   //
   // BLOCKCHAIN REORGANIZATION UNIT-TEST
   //
   /////////////////////////////////////////////////////////////////////////////
   //
   // NOTE:  The unit-test files (blk_0_to_4, blk_3A, etc) are located in 
   //        cppForSwig/reorgTest.  These files represent a very small 
   //        blockchain with a double-spend and a couple invalidated 
   //        coinbase tx's.  All tx-hashes & OutPoints are consistent,
   //        all transactions have real ECDSA signatures, and blockheaders
   //        have four leading zero-bytes to be valid at difficulty=1
   //        
   //        If you were to set COINBASE_MATURITY=1 (not applicable here)
   //        this would be a *completely valid* blockchain--just a very 
   //        short blockchain.
   //
   //        FYI: The first block is the *actual* main-network genesis block
   //
   string blk04("reorgTest/blk_0_to_4.dat");
   string blk3A("reorgTest/blk_3A.dat");
   string blk4A("reorgTest/blk_4A.dat");
   string blk5A("reorgTest/blk_5A.dat");

   BtcWallet wlt2;
   wlt2.addAddress(BinaryData::CreateFromHex("62e907b15cbf27d5425399ebf6f0fb50ebb88f18"));
   wlt2.addAddress(BinaryData::CreateFromHex("ee26c56fc1d942be8d7a24b2a1001dd894693980"));
   wlt2.addAddress(BinaryData::CreateFromHex("cb2abde8bccacc32e893df3a054b9ef7f227a4ce"));
   wlt2.addAddress(BinaryData::CreateFromHex("c522664fb0e55cdc5c0cea73b4aad97ec8343232"));

                   
   cout << endl << endl;
   cout << "Preparing blockchain-reorganization test!" << endl;
   cout << "Resetting block-data mgr...";
   bdm.Reset();
   cout << "Done!" << endl;
   cout << "Reading in initial block chain (Blocks 0 through 4)..." ;
   bdm.readBlkFile_FromScratch("reorgTest/blk_0_to_4.dat");
   bdm.organizeChain();
   cout << "Done" << endl;

   // TODO: Let's look at the address ledger after the first chain
   //       Then look at it again after the reorg.  What we want
   //       to see is the presence of an invalidated tx, not just
   //       a disappearing tx -- the user must be informed that a 
   //       tx they previously thought they owned is now invalid.
   //       If the user is not informed, they could go crazy trying
   //       to figure out what happened to this money they thought
   //       they had.
   cout << "Constructing address ledger for the to-be-invalidated chain:" << endl;
   bdm.scanBlockchainForTx_FromScratch(wlt2);
   vector<LedgerEntry> const & ledgerAll2 = wlt2.getTxLedger();
   for(uint32_t j=0; j<ledgerAll2.size(); j++)
   {  
      cout << "    Tx: " 
           << ledgerAll2[j].getValue()/1e8
           << " (" << ledgerAll2[j].getBlockNum() << ")"
           << "  TxHash: " << ledgerAll2[j].getTxHash().getSliceCopy(0,4).toHexStr();
      if(!ledgerAll2[j].isValid())      cout << " (INVALID) ";
      if( ledgerAll2[j].isSentToSelf()) cout << " (SENT_TO_SELF) ";
      if( ledgerAll2[j].isChangeBack()) cout << " (RETURNED CHANGE) ";
      cout << endl;
   }
   cout << "Checking balance of all addresses: " << wlt2.getNumAddr() << "addrs" << endl;
   cout << "                          Balance: " << wlt2.getBalance()/1e8 << endl;
   for(uint32_t i=0; i<wlt2.getNumAddr(); i++)
   {
      BinaryData addr20 = wlt2.getAddrByIndex(i).getAddrStr20();
      cout << "  Addr: " << wlt2.getAddrByIndex(i).getBalance()/1e8 << ","
                         << wlt2.getAddrByHash160(addr20).getBalance() << endl;
      vector<LedgerEntry> const & ledger = wlt2.getAddrByIndex(i).getTxLedger();
      for(uint32_t j=0; j<ledger.size(); j++)
      {  
         cout << "    Tx: " 
              << ledger[j].getAddrStr20().getSliceCopy(0,4).toHexStr() << "  "
              << ledger[j].getValue()/(float)(CONVERTBTC) << " (" 
              << ledger[j].getBlockNum()
              << ")  TxHash: " << ledger[j].getTxHash().getSliceCopy(0,4).toHexStr();
         if( ! ledger[j].isValid())  cout << " (INVALID) ";
         cout << endl;
      }

   }

   // prepare the other block to be read in
   ifstream is;
   BinaryData blk3a, blk4a, blk5a;
   assert(blk3a.readBinaryFile("reorgTest/blk_3A.dat") != -1);
   assert(blk4a.readBinaryFile("reorgTest/blk_4A.dat") != -1);
   assert(blk5a.readBinaryFile("reorgTest/blk_5A.dat") != -1);
   vector<bool> result;

   /////
   cout << "Pushing Block 3A into the BDM:" << endl;
   result = bdm.addNewBlockData(blk3a);

   /////
   cout << "Pushing Block 4A into the BDM:" << endl;
   result = bdm.addNewBlockData(blk4a);

   /////
   cout << "Pushing Block 5A into the BDM:" << endl;
   result = bdm.addNewBlockData(blk5a);
   if(result[ADD_BLOCK_CAUSED_REORG] == true)
   {
      cout << "Reorg happened after pushing block 5A" << endl;
      bdm.scanBlockchainForTx_FromScratch(wlt2);
      bdm.updateWalletAfterReorg(wlt2);
   }

   cout << "Checking balance of entire wallet: " << wlt2.getBalance()/1e8 << endl;
   vector<LedgerEntry> const & ledgerAll3 = wlt2.getTxLedger();
   for(uint32_t j=0; j<ledgerAll3.size(); j++)
   {  
      cout << "    Tx: " 
           << ledgerAll3[j].getValue()/1e8
           << " (" << ledgerAll3[j].getBlockNum() << ")"
           << "  TxHash: " << ledgerAll3[j].getTxHash().getSliceCopy(0,4).toHexStr();
      if(!ledgerAll3[j].isValid())      cout << " (INVALID) ";
      if( ledgerAll3[j].isSentToSelf()) cout << " (SENT_TO_SELF) ";
      if( ledgerAll3[j].isChangeBack()) cout << " (RETURNED CHANGE) ";
      cout << endl;
   }

   cout << "Checking balance of all addresses: " << wlt2.getNumAddr() << "addrs" << endl;
   for(uint32_t i=0; i<wlt2.getNumAddr(); i++)
   {
      BinaryData addr20 = wlt2.getAddrByIndex(i).getAddrStr20();
      cout << "  Addr: " << wlt2.getAddrByIndex(i).getBalance()/1e8 << ","
                         << wlt2.getAddrByHash160(addr20).getBalance()/1e8 << endl;
      vector<LedgerEntry> const & ledger = wlt2.getAddrByIndex(i).getTxLedger();
      for(uint32_t j=0; j<ledger.size(); j++)
      {  
         cout << "    Tx: " 
              << ledger[j].getAddrStr20().getSliceCopy(0,4).toHexStr() << "  "
              << ledger[j].getValue()/(float)(CONVERTBTC) << " (" 
              << ledger[j].getBlockNum()
              << ")  TxHash: " << ledger[j].getTxHash().getSliceCopy(0,4).toHexStr();
         if( ! ledger[j].isValid())
            cout << " (INVALID) ";
         cout << endl;
      }

   }
   /////////////////////////////////////////////////////////////////////////////
   //
   // END BLOCKCHAIN REORG UNIT-TEST
   //
   /////////////////////////////////////////////////////////////////////////////
  

}



void TestCrypto(void)
{

   /////////////////////////////////////////////////////////////////////////////
   // Start Key-Derivation-Function (KDF) Tests.  
   // ROMIX is the provably memory-hard (GPU-resistent) algorithm proposed by 
   // Colin Percival, who is the creator of Scrypt.  
   cout << endl << endl;
   cout << "Executing Key-Derivation-Function (KDF) tests" << endl;
   KdfRomix kdf;  
   kdf.computeKdfParams();
   kdf.printKdfParams();

   BinaryData passwd1("This is my first password");
   BinaryData passwd2("This is my first password.");
   BinaryData passwd3("This is my first password");
   SensitiveKeyData key;

   cout << "   Password1: '" << passwd1.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd1);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   cout << "   Password2: '" << passwd2.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd2);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   cout << "   Password1: '" << passwd3.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd3);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   /////////////////////////////////////////////////////////////////////////////
   cout << "Executing KDF tests with longer compute time" << endl;
   kdf.computeKdfParams(1.0);
   kdf.printKdfParams();

   cout << "   Password1: '" << passwd1.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd1);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   cout << "   Password2: '" << passwd2.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd2);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   cout << "   Password1: '" << passwd3.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd3);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   /////////////////////////////////////////////////////////////////////////////
   cout << "Executing KDF tests with limited memory target" << endl;
   kdf.computeKdfParams(1.0, 256*1024);
   kdf.printKdfParams();

   cout << "   Password1: '" << passwd1.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd1);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   cout << "   Password2: '" << passwd2.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd2);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   cout << "   Password1: '" << passwd3.toBinStr() << "'" << endl;
   key = kdf.DeriveKey(passwd3);
   cout << "   MasterKey: '" << key.toHexStr() << endl << endl;

   // Test AES code using NIST test vectors
   
   /// *** Test 1 *** ///
   cout << endl << endl;
   BinaryData testIV, plaintext, ciphertext;
   SensitiveKeyData testKey(BinaryData::CreateFromHex( "0000000000000000000000000000000000000000000000000000000000000000"));
   testIV.createFromHex    ("80000000000000000000000000000000");
   plaintext.createFromHex ("00000000000000000000000000000000");
   ciphertext.createFromHex("ddc6bf790c15760d8d9aeb6f9a75fd4e");
   CryptoAES().EncryptInPlace(plaintext, testKey, testIV);
   cout << "   Answer    : " << ciphertext.toHexStr() << endl;
   CryptoAES().DecryptInPlace(plaintext, testKey, testIV);


   /// *** Test 2 *** ///
   cout << endl << endl;
   testKey.setKeyData(BinaryData::CreateFromHex( "0000000000000000000000000000000000000000000000000000000000000000"));
   testIV.createFromHex    ("014730f80ac625fe84f026c60bfd547d");
   plaintext.createFromHex ("00000000000000000000000000000000");
   ciphertext.createFromHex("5c9d844ed46f9885085e5d6a4f94c7d7");
   CryptoAES().EncryptInPlace(plaintext, testKey, testIV);
   cout << "   Answer    : " << ciphertext.toHexStr() << endl;
   CryptoAES().DecryptInPlace(plaintext, testKey, testIV);


   /// *** Test 3 *** ///
   cout << endl << endl;
   testKey.setKeyData(BinaryData::CreateFromHex( "ffffffffffff0000000000000000000000000000000000000000000000000000"));
   testIV.createFromHex    ("00000000000000000000000000000000");
   plaintext.createFromHex ("00000000000000000000000000000000");
   ciphertext.createFromHex("225f068c28476605735ad671bb8f39f3");
   CryptoAES().EncryptInPlace(plaintext, testKey, testIV);
   cout << "   Answer    : " << ciphertext.toHexStr() << endl;
   CryptoAES().DecryptInPlace(plaintext, testKey, testIV);


   /// My own test, for sanity (can only check the roundtrip values)
   cout << endl << endl;
   cout << "Starting some kdf-aes-combined tests..." << endl;
   kdf.printKdfParams();
   testKey = kdf.DeriveKey(BinaryData("This passphrase is tough to guess"));
   BinaryData secret;
   secret.createFromHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
   BinaryData randIV(0);  // tell the crypto to generate a random IV for me.

   cout << "Encrypting:" << endl;
   CryptoAES().EncryptInPlace(secret, testKey, randIV);
   cout << endl << endl;
   cout << "Decrypting:" << endl;
   CryptoAES().DecryptInPlace(secret, testKey, randIV);
   cout << endl << endl;

   // Now encrypting so I can store the encrypted data in file
   cout << "Encrypting again:" << endl;
   CryptoAES().EncryptInPlace(secret, testKey, randIV);

   ofstream testfile("safefile.txt", ios::out);
   testfile << "KdfParams " << endl;
   testfile << "   MemReqts " << kdf.getMemoryReqtBytes() << endl;
   testfile << "   NumIters " << kdf.getNumIterations() << endl;
   testfile << "   HexSalt  " << kdf.getSalt().toHexStr() << endl;
   testfile << "EncryptedData" << endl;
   testfile << "   HexIV    " << randIV.toHexStr() << endl;
   testfile << "   Cipher   " << secret.toHexStr() << endl;
   testfile.close();
   
   ifstream infile("safefile.txt", ios::in);
   uint32_t mem, nIters;
   BinaryData salt, iv, cipher;
   char deadstr[256];
   char hexstr[256];

   infile >> deadstr;
   infile >> deadstr >> mem;
   infile >> deadstr >> nIters;
   infile >> deadstr >> hexstr;
   salt.copyFrom( BinaryData::CreateFromHex(string(hexstr, 64)));
   infile >> deadstr;
   infile >> deadstr >> hexstr;
   iv.copyFrom( BinaryData::CreateFromHex(string(hexstr, 64)));
   infile >> deadstr >> hexstr;
   cipher.copyFrom( BinaryData::CreateFromHex(string(hexstr, 64)));
   infile.close();
   cout << endl << endl;

   // Will try this twice, once with correct passphrase, once without
   BinaryData cipherTry1 = cipher;
   BinaryData cipherTry2 = cipher;
   SensitiveKeyData newKey;

   KdfRomix newKdf(mem, nIters, salt);
   newKdf.printKdfParams();

   // First test with the wrong passphrase
   cout << "Attempting to decrypt with wrong passphrase" << endl;
   BinaryData passphrase = BinaryData("This is the wrong passphrase");
   newKey = newKdf.DeriveKey( passphrase );
   CryptoAES().DecryptInPlace(cipherTry1, newKey, iv);


   // Now try correct passphrase
   cout << "Attempting to decrypt with CORRECT passphrase" << endl;
   passphrase = BinaryData("This passphrase is tough to guess");
   newKey = newKdf.DeriveKey( passphrase );
   CryptoAES().DecryptInPlace(cipherTry2, newKey, iv);
}

