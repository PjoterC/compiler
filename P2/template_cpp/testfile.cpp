struct A {                                  // visitPDefs A, visitDStruct A
    int x;                                 // visitListField, visitFDecl, visitType_int
    int y;                                 // visitListField, visitFDecl, visitType_int
};

struct B : A {                             // visitPDefs B, visitDStructDer B
    double d;                              // visitListField, visitFDecl, visitType_double
};


int f(int a) {                              // visitPDefs f, visitDFun f, visitADecl a, visitType_int, visitId a
    int i = a;                              // visitSDecls i, visitType_int, visitEId a, visitId a
    i++;                                    // visitEPIncr, visitEId i
    i--;                                    // visitEPDecr, visitEId i
    +i;                                     // visitEUPlus, visitEId i
    -i;                                     // visitEUMinus, visitEId i

    A a2;                                   // visitSDecls a2, visitTypeId A
    a2.x = i;                              // visitEProj, visitEId a2, visitId x, visitEId i

    return i;                              // visitSReturnV, visitEId i
}

void main1() {                               // visitPDefs main, visitDFun main
    int k;                                  // visitSDecls k, visitType_int
    k = 5;                                  // visitEAss, visitEId k, visitEInt 5

    bool b = true;                          // visitSDecls b, visitType_bool, visitETrue

    f(k);                                   // visitSExp, visitEApp f, visitId f, visitEId k

    try {                                    // visitSTry
        throw k;                             // visitEThrow, visitEId k
    } catch (A a3) {                         // visitSTry type: visitTypeId A, visitIdNoInit a3
        a3.y = 2;                           // visitEProj, visitEId a3, visitId y, visitEInt 2
    }
}


/*
visitPDefs A
visitPDefs f
visitPDefs main1
visitDStruct A
visitListField0x1efea70
visitFDecl x
visitType_int0x1ef9890
visitId x
visitFDecl y
visitType_int0x1ef9890
visitId y
visitDStructDer B
visitId B
Visiting TypeId: A
visitListField0x1efec10
visitFDecl d
visitType_double0x1ef9940
visitId d
visitDFun f
visitADecl a
visitType_int0x1ef9890
visitId a
visitSDecls 0x1efede0
visitType_int0x1ef9890
visitSExp 0x1efef90
visitEPIncr 0x1efef60
visitEId i
visitId i
visitSExp 0x1eff020
visitEPDecr 0x1efeff0
visitEId i
visitId i
visitSExp 0x1eff0c0
visitEUPlus 0x1eff090
visitEId i
visitId i
visitSExp 0x1eff150
visitEUMinus 0x1eff120
visitEId i
visitId i
visitSDecls 0x1eff060
Visiting TypeId: A
visitSExp 0x1eff3c0
visitEAss 0x1eff330, 0x1eff390
visitEProj x
visitEId a2
visitId a2
visitId x
visitEId i
visitId i
visitSReturn 0x1eff420
visitDFun main1
visitSDecls 0x1eff570
visitType_int0x1ef9890
visitSExp 0x1eff6e0
visitEAss 0x1eff690, 0x1eff6c0
visitEId k
visitId k
visitEInt 5
visitInteger
visitSDecls 0x1eff650
visitType_bool0x1ef9900
visitSExp 0x1eff900
visitEApp f
visitId f
visitListExp0x1eff8b0
visitEId k
visitId k
visitSTry 0x1effa40
visitSBlock 0x1eff960
visitListStm0x1eff960
visitSExp 0x1eff9e0
visitEThrow 0x1eff9b0
visitEId k
visitId k
visitSTry type: 0x1effa80
Visiting TypeId: A
Binding catch variable: a3 of type 0x1effa80
visitSTry catch block: 0x1effc30
visitSBlock 0x1effad0
visitListStm0x1effad0
visitSExp 0x1effbd0
visitEAss 0x1effb70, 0x1effbb0
visitEProj y
visitEId a3
visitId a3
visitId y
visitEInt 2
visitInteger
Type checking completed successfully.
*/